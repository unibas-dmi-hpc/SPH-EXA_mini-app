#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// hard code MPI for now
#define USE_MPI

#include "cstone/domain.hpp"
#include "gravity.hpp"

#include "sphexa.hpp"
#include "EvrardCollapseInputFileReader.hpp"
#include "EvrardCollapseFileWriter.hpp"

#include "sph/findNeighborsSfc.hpp"

using namespace sphexa;
using namespace cstone;

void printHelp(char *binName, int rank);

int main(int argc, char **argv)
{
    const int rank = initAndGetRankId();

    const ArgParser parser(argc, argv);

    if (parser.exists("-h") || parser.exists("--h") || parser.exists("-help") || parser.exists("--help"))
    {
        printHelp(argv[0], rank);
        return exitSuccess();
    }

    const size_t maxStep = parser.getInt("-s", 10);
    const size_t nParticles = parser.getInt("-n", 65536);
    const int writeFrequency = parser.getInt("-w", -1);
    const int checkpointFrequency = parser.getInt("-c", -1);
    const bool quiet = parser.exists("--quiet");
    const bool timeRemoteGravitySteps = parser.exists("--timeRemoteGravity");
    const std::string checkpointInput = parser.getString("--cinput");
    const std::string inputFilePath = parser.getString("--input", "bigfiles/Test3DEvrardRel.bin");
    const std::string outDirectory = parser.getString("--outDir");

    std::ofstream nullOutput("/dev/null");
    std::ostream &output = quiet ? nullOutput : std::cout;

    using Real = double;
    using CodeType = unsigned;
    using Dataset = GravityParticlesData<Real>;

    const IFileReader<Dataset> &fileReader = EvrardCollapseMPIInputFileReader<Dataset>();
    const IFileWriter<Dataset> &fileWriter = EvrardCollapseMPIFileWriter<Dataset>();
    auto d = checkpointInput.empty() ? fileReader.readParticleDataFromBinFile(inputFilePath, nParticles)
                                     : fileReader.readParticleDataFromCheckpointBinFile(checkpointInput);

    const Printer<Dataset> printer(d);

    if(d.rank == 0) std::cout << "Data generated." << std::endl;

    MasterProcessTimer timer(output, d.rank), totalTimer(output, d.rank);

    std::ofstream constantsFile(outDirectory + "constants.txt");

    // -n 350, 42M per node
    const int bucketSize = 2048;
    cstone::Box<Real> box{d.bbox.xmin, d.bbox.xmax, d.bbox.ymin, d.bbox.ymax,
                          d.bbox.zmin, d.bbox.zmax, d.bbox.PBCx, d.bbox.PBCy, d.bbox.PBCz};
    cstone::Domain<CodeType, Real> domain(rank, d.nrank, bucketSize, box);

    if(d.rank == 0) std::cout << "Domain created." << std::endl;

    std::vector<CodeType> codes;
    domain.sync(d.x, d.y, d.z, d.h, codes, d.m, d.mui, d.u, d.vx, d.vy, d.vz,
                d.x_m1, d.y_m1, d.z_m1, d.du_m1, d.dt_m1);

    std::vector<int> clist(domain.nParticles());
    std::iota(begin(clist), end(clist), domain.startIndex());

    if(d.rank == 0) std::cout << "Domain synchronized, nLocalParticles " << d.x.size() << std::endl;

    const size_t nTasks = 64;
    const size_t ngmax = 150;
    const size_t ng0 = 100;
    TaskList taskList = TaskList(clist, nTasks, ngmax, ng0);

    if(d.rank == 0) std::cout << "Starting main loop." << std::endl;

    totalTimer.start();
    for (d.iteration = 0; d.iteration <= maxStep; d.iteration++)
    {
        timer.start();
        domain.sync(d.x, d.y, d.z, d.h, codes, d.m, d.mui, d.u, d.vx, d.vy, d.vz,
                    d.x_m1, d.y_m1, d.z_m1, d.du_m1, d.dt_m1);
        domain.exchangeHalos(d.m);
        timer.step("domain::sync");
        d.resize(d.x.size());  // also resize arrays not listed in sync, even though space for halos is not needed
        clist.resize(domain.nParticles());
        std::iota(begin(clist), end(clist), domain.startIndex());
        // TODO: build the local gravity tree
        // build the global gravity tree
        gravity::buildGlobalGravityTree(domain.tree(), d.x, d.y, d.z, d.m, codes, domain.box(), domain.gTree(), false);
        timer.step("gravity::buildGravityTree");
        taskList.update(clist);
        timer.step("updateTasks");
        // DEBUG TREE
        gravity::showParticles(domain.tree(), d.x, d.y, d.z, d.m, codes, domain.box());
        // END DEBUG TREE
        // gravity treewalk
        auto rankToParticlesForRemoteGravCalculations = gravity::gravityTreeWalk(taskList.tasks, domain.gTree(), d);
        timer.step("Gravity (self)");
        // remote gravity treewalk
        // mpi barrier
        sph::findNeighborsSfc(taskList.tasks, d.x, d.y, d.z, d.h, codes, domain.box());
        timer.step("FindNeighbors");
        if(!clist.empty()) sph::computeDensity<Real>(taskList.tasks, d);
        timer.step("Density");
        sph::computeEquationOfStateEvrard<Real>(taskList.tasks, d);
        timer.step("EquationOfState");
        domain.exchangeHalos(d.vx, d.vy, d.vz, d.ro, d.p, d.c);
        timer.step("mpi::synchronizeHalos");
        if(!clist.empty()) sph::computeIAD<Real>(taskList.tasks, d);
        timer.step("IAD");
        domain.exchangeHalos(d.c11, d.c12, d.c13, d.c22, d.c23, d.c33);
        timer.step("mpi::synchronizeHalos");
        if(!clist.empty()) sph::computeMomentumAndEnergyIAD<Real>(taskList.tasks, d);
        timer.step("MomentumEnergyIAD");
        sph::computeTimestep<Real, sph::TimestepPress2ndOrder<Real, Dataset>>(taskList.tasks, d);
        timer.step("Timestep"); // AllReduce(min:dt)
        sph::computePositions<Real, sph::computeAcceleration<Real, Dataset>>(taskList.tasks, d);
        timer.step("UpdateQuantities");
        sph::computeTotalEnergy<Real>(taskList.tasks, d);
        timer.step("EnergyConservation"); // AllReduce(sum:ecin,ein)
        sph::updateSmoothingLength<Real>(taskList.tasks, d);
        timer.step("UpdateSmoothingLength");

        const size_t totalNeighbors = sph::neighborsSum(taskList.tasks);

        if (d.rank == 0)
        {
            printer.printCheck(domain.nParticles(), domain.tree().size(), d.x.size() - domain.nParticles(), totalNeighbors, output);
            printer.printConstants(d.iteration, totalNeighbors, constantsFile);
        }

        if ((writeFrequency > 0 && d.iteration % writeFrequency == 0) || writeFrequency == 0)
        {
            fileWriter.dumpParticleDataToAsciiFile(d, clist, outDirectory + "dump_Sedov" + std::to_string(d.iteration) + ".txt");
            fileWriter.dumpParticleDataToBinFile(d, outDirectory + "dump_Sedov" + std::to_string(d.iteration) + ".bin");
            timer.step("writeFile");
        }

        timer.stop();

        if (d.rank == 0) printer.printTotalIterationTime(timer.duration(), output);
    }

    totalTimer.step("Total execution time of " + std::to_string(maxStep) + " iterations of Sedov");

    constantsFile.close();

    return exitSuccess();
}

void printHelp(char *name, int rank)
{
    if (rank == 0)
    {
        printf("\nUsage:\n\n");
        printf("%s [OPTIONS]\n", name);
        printf("\nWhere possible options are:\n");
        printf("\t-n NUM \t\t\t NUM^3 Number of particles\n");
        printf("\t-s NUM \t\t\t NUM Number of iterations (time-steps)\n");
        printf("\t-w NUM \t\t\t Dump particles data every NUM iterations (time-steps)\n\n");

        printf("\t--quiet \t\t Don't print anything to stdout\n\n");

        printf("\t--outDir PATH \t\t Path to directory where output will be saved.\
                    \n\t\t\t\t Note that directory must exist and be provided with ending slash.\
                    \n\t\t\t\t Example: --outDir /home/user/folderToSaveOutputFiles/\n");
    }
}