#include <chrono>
#include <rng.h>
#include <vptree.h>
#include <distance_matrix.h>
#include <deque>
#include <iostream>
#include <fstream>
#include <memory>
#include <iomanip>

#define DECLARE_TIMER std::chrono::time_point<std::chrono::high_resolution_clock> __start_timer, __stop_timer; std::size_t __integral_time; 
#define START_TIMER __start_timer = std::chrono::high_resolution_clock::now(); \
                    std::cout << std::flush

#define END_TIMER __stop_timer = std::chrono::high_resolution_clock::now(); \
                  __integral_time = static_cast<std::size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(__stop_timer - __start_timer).count()); \
                  std::cout << std::setprecision(3) << (__integral_time / 1000.0) << "s" << std::endl

namespace Reorder 
{    
    struct
    {
        unsigned index;
        double distance;
    } typedef IndexDistance;


    std::string executeCommand(const std::string& command)
    {
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        
        std::string result;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
            result += buffer;
        }
        
        return result;
    }

    double compute_distance(const std::string& a, const std::string& b)
    {
        std::string str_result = executeCommand("mash dist " + a + " " + b + " 2>/dev/null | cut -f3");
        return std::stod(str_result);
    }


    IndexDistance find_closest_vertex(VPTree<unsigned>& VPTREE, const unsigned VERTEX, const std::vector<bool>& ALREADY_ADDED)
    {
        IndexDistance nn = {0, 2.0};
        VPTREE.getUnvisitedNearestNeighbor(VERTEX, ALREADY_ADDED, &nn.distance, &nn.index);

        return nn;
    }

    //TSP path
    void build_NN(DistanceMatrix& distanceMatrix, const std::vector<std::string>& FILES, const std::size_t OFFSET, std::vector<unsigned>& order)
    {
        //Pick a random first vertex
        unsigned firstVertex = RNG::rand_uint32_t(0, distanceMatrix.width());
        
        //Vector of added vertices (set true for the first vertex)
        std::vector<bool> alreadyAdded;
        alreadyAdded.resize(distanceMatrix.width());
        alreadyAdded[firstVertex] = true;

        //Deque for building path with first vertex as starting point
        std::vector<unsigned> path = {firstVertex};
        path.reserve(distanceMatrix.width());

        //Build vector of indices for VPTree
        std::vector<unsigned> vertices;
        vertices.resize(distanceMatrix.width());
        for(unsigned i = 0; i < vertices.size(); ++i)
            vertices[i] = i;

        unsigned counter = 0;

        DistanceFunctions<unsigned> df = VPTree<unsigned>::bindDistanceFunctions(
            [&](unsigned a, unsigned b) -> double {
                ++counter;
                return compute_distance(FILES[a], FILES[b]);
            },
            [&distanceMatrix](unsigned a, unsigned b) -> double { return distanceMatrix.get(a, b); },
            [&distanceMatrix](unsigned a, unsigned b, double d) { distanceMatrix.set(a, b, d); }
        );

        VPTree<unsigned> root(vertices, &df);

        //Added second vertex to data structures
        //Find next vertices to add by checking which is the minimum to take
        for(unsigned i = 1; i < distanceMatrix.width(); ++i)
        {
            IndexDistance match = find_closest_vertex(root, path[i-1], alreadyAdded);
            alreadyAdded[match.index] = true;
            path.push_back(match.index);
        }

        std::cout << "\tComputed distances (VPTree): " << counter << "/" << (distanceMatrix.width() * (distanceMatrix.width() - 1) / 2) <<  std::endl;

        //Store global order
        for(unsigned i = 0; i < distanceMatrix.width(); ++i)
            order[i+OFFSET] = path[i] + OFFSET; //Add offset because columns are addressed by their global location
    }


    //TSP path
    void build_double_end_NN(DistanceMatrix& distanceMatrix, const std::vector<std::string>& FILES, const std::size_t OFFSET, std::vector<unsigned>& order)
    {
        //Pick a random first vertex
        unsigned firstVertex = RNG::rand_uint32_t(0, distanceMatrix.width());
        
        //Vector of added vertices (set true for the first vertex)
        std::vector<bool> alreadyAdded;
        alreadyAdded.resize(distanceMatrix.width());
        alreadyAdded[firstVertex] = true;

        //Deque for building path with first vertex as starting point
        std::deque<unsigned> orderDeque = {firstVertex};

        //Build vector of indices for VPTree
        std::vector<unsigned> vertices;
        vertices.resize(distanceMatrix.width());
        for(unsigned i = 0; i < vertices.size(); ++i)
            vertices[i] = i;

        unsigned counter = 0;

        //Use counter to count how many distance computation could be avoided by using a VPTree
        DistanceFunctions<unsigned> df = VPTree<unsigned>::bindDistanceFunctions(
            [&](unsigned a, unsigned b) -> double {
                ++counter;
                return compute_distance(FILES[a], FILES[b]);
            },
            [&distanceMatrix](unsigned a, unsigned b) -> double { return distanceMatrix.get(a, b); },
            [&distanceMatrix](unsigned a, unsigned b, double d) { distanceMatrix.set(a, b, d); }
        );

        std::cout << "Build VPTree.." << std::endl;

        VPTree<unsigned> root(vertices, &df);

        //Find second vertex
        IndexDistance second = find_closest_vertex(root, firstVertex, alreadyAdded);
        
        //Added second vertex to data structures
        orderDeque.push_back(second.index);
        alreadyAdded[second.index] = true;

        //Find closest vertices from path front and back
        IndexDistance a = find_closest_vertex(root, orderDeque.front(), alreadyAdded);
        IndexDistance b = find_closest_vertex(root, orderDeque.back(), alreadyAdded);

        std::cout << "Build path.." << std::endl;
        //Find next vertices to add by checking which is the minimum to take
        for(unsigned i = 2; i < distanceMatrix.width(); ++i)
        {
            //std::cout << "\r" << (i+1) << " / " << distanceMatrix.width();
            if(a.distance < b.distance)
            {
                orderDeque.push_front(a.index);
                alreadyAdded[a.index] = true;

                if(a.index == b.index)
                    b = find_closest_vertex(root, orderDeque.back(), alreadyAdded);

                a = find_closest_vertex(root, orderDeque.front(), alreadyAdded);
            }
            else
            {
                orderDeque.push_back(b.index);
                alreadyAdded[b.index] = true;

                if(b.index == a.index)
                    a = find_closest_vertex(root, orderDeque.front(), alreadyAdded);

                b = find_closest_vertex(root, orderDeque.back(), alreadyAdded);
            }
        }

        std::cout << "\tComputed distances (VPTree): " << counter << "/" << (distanceMatrix.width() * (distanceMatrix.width() - 1) / 2) <<  std::endl;

        //Store global order
        for(unsigned i = 0; i < distanceMatrix.width(); ++i)
            order[i+OFFSET] = orderDeque[i] + OFFSET; //Add offset because columns are addressed by their global location
    }


    void TSP_NN(const std::vector<std::string>& FILES, const unsigned GROUPSIZE, std::vector<unsigned>& order)
    {
        unsigned NB_ELEMENTS = FILES.size();

        unsigned last_group_size;
        
        //Number of group of columns
        const unsigned NB_GROUPS = (NB_ELEMENTS+GROUPSIZE-1)/GROUPSIZE;

        if(NB_ELEMENTS % GROUPSIZE == 0)
            last_group_size = GROUPSIZE;
        else
            last_group_size = NB_ELEMENTS % GROUPSIZE;

        std::size_t offset = 0; //Offset for global order assignation

        DistanceMatrix distanceMatrix(GROUPSIZE);
        for(unsigned i = 0; i < NB_GROUPS-1; ++i)
        {
            //Find a suboptimal path minimizing the weight of edges and visiting each node once
            build_double_end_NN(distanceMatrix, FILES, offset, order);
            
            offset += GROUPSIZE;
            distanceMatrix.reset(); //Clean distance matrix
        }

        distanceMatrix.resize(last_group_size);
        build_double_end_NN(distanceMatrix, FILES, offset, order);
    }
    
    void launch(const std::vector<std::string>& files_fa, const std::vector<std::string>& files_msh, const unsigned GROUPSIZE, const std::string& OUTPUT)
    {
        DECLARE_TIMER;

        std::vector<unsigned> order;
        order.resize(files_fa.size());

        //Approximate TSP path with double ended Nearest-Neighbor; compute order
        std::cout << "Computing column order using TSP ... " << std::endl;
        START_TIMER;
        TSP_NN(files_msh, GROUPSIZE, order);
        END_TIMER;

        //Serialize column order
        std::ofstream fdout(OUTPUT);

        if(!fdout.good())
            throw std::runtime_error("Couldn't open ouput FOF");

        for(std::size_t i = 0; i < files_fa.size(); ++i)
            fdout << files_fa[order[i]] << "\n";

        fdout.close();
    }

}

int main(int argc, char ** argv)
{
    if(argc != 5)
    {
        std::cout << "Usage: reorder <fof_fa> <fof_msh> <group> <output>\n\n";
        return 1;
    }

    RNG::set_seed(42);

    std::ifstream fof_fa(argv[1]);
    std::ifstream fof_msh(argv[2]);

    //Check if file was successfully opened
    if(!fof_fa.good())
        throw std::runtime_error("Couldn't open FOF with FASTA files");

    if(!fof_msh.good())
        throw std::runtime_error("Couldn't open FOF with MSH files");


    unsigned groupsize = (unsigned)atol(argv[3]);

    std::string output = argv[4];

    std::string line;
    std::vector<std::string> files_fa;
    std::vector<std::string> files_msh;

    //Store all string
    while (std::getline(fof_fa, line))
        files_fa.push_back(line);

    files_msh.reserve(files_fa.size());

    //Store all string
    while (std::getline(fof_msh, line))
        files_msh.push_back(line);

    fof_fa.close();
    fof_msh.close();

    if(files_fa.size() != files_msh.size())
        throw std::runtime_error("Both FOF don't have same size");

    if(groupsize == 0)
        groupsize = files_fa.size();

    Reorder::launch(files_fa, files_msh, groupsize, output);
}
