#pragma once
#define IGNORE_JULIENNE_TYPES
#include <infra_hb/host/graphhb_blocking.hpp>
#include <infra_hb/host/wgraphhb.hpp>
#include <infra_hb/host/device.hpp>
#include <infra_hb/host/error.hpp>
#include <infra_hb/host/global_scalar.hpp>
#include <infra_hb/host/priority_queue.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <sys/stat.h>
#include "intrinsics.h"
#include <string>

#define SIM_PARTITIONING

namespace hammerblade {

static
GraphHB builtin_loadEdgesFromFileToHB(const char *graph_file)
{
        if (!graph_file)
                throw std::runtime_error("bad graph file: did you pass a graph argument?");
        std::cout << "Enter the GraphHB constructor function" << std::endl;
        return GraphHB(builtin_loadEdgesFromFile(graph_file));
}

static
WGraphHB builtin_loadWeightedEdgesFromFileToHB(const char *graph_file)
{
        if (!graph_file)
                throw std::runtime_error("bad graph file: did you pass a graph argument?");

        return WGraphHB(builtin_loadWeightedEdgesFromFile(graph_file));
}

static
int * builtin_loadFrontierFromFile(const char *frontier_file)
{
        std::vector<int> file_vals;
        std::string   line;
        ifstream file (frontier_file);
        while(std::getline(file, line))
        {
          file_vals.push_back(std::stoi(line));
        }
        int * arr = new int[file_vals.size()];
        std::copy(file_vals.begin(), file_vals.end(), arr);
        return arr;

}

static
GraphHB builtin_transposeHB(GraphHB & graph)
{
        return GraphHB(builtin_transpose(graph.getHostGraph()));
}

static
void builtin_loadMicroCodeFromSTDVectorRValue(std::vector<unsigned char> &&ucode)
{
        Device::Ptr device = Device::GetInstance();
        device->setMicroCode(std::move(ucode));
}

static
void builtin_loadMicroCodeFromSTDVectorCopy(const std::vector<unsigned char> &ucode)
{
        Device::Ptr device = Device::GetInstance();
        std::vector<unsigned char> ucode_cpy = ucode;
        device->setMicroCode(std::move(ucode_cpy));
}

static
void builtin_loadMicroCodeFromSTDVector(std::vector<unsigned char> &ucode)
{
        Device::Ptr device = Device::GetInstance();
        device->setMicroCode(std::move(ucode));
}

static
void builtin_loadMicroCodeFromFile(const std::string &ucode_fname)
{
        /* read in the micro code */
        struct stat st;
        int err = stat(ucode_fname.c_str(), &st);
        if (err != 0) {
                stringstream error;
                error << "Failed to stat micro-code file '" << ucode_fname << "': "
                      << std::string(strerror(errno)) << std::endl;
                throw hammerblade::runtime_error(error.str());
        }

        std::vector<unsigned char> ucode(st.st_size, 0);
        std::fstream ucode_file (ucode_fname);
        ucode_file.read((char*)ucode.data(), ucode.size());

        /* load micro code to device */
        builtin_loadMicroCodeFromSTDVectorRValue(std::move(ucode));
}

//TODO(Emily): ideally this computation would happen on the device
//             so that we can avoid this unnecessary copy
static
int builtin_getVertexSetSizeHB(Vector<int32_t> &frontier, int len){
    int size = 0;
    int32_t temp[len];
    frontier.copyToHost(temp, len);
    for(auto i : temp) {
      if(i == 1) {
        size++;
      }
    }
    return size;
}

static
void builtin_addVertexHB(Vector<int32_t> &frontier, int pos)
{
  frontier.insert(pos, 1);
}

static
int builtin_getVerticesHB(GraphHB &g)
{
  return g.num_nodes();
}

static
int builtin_getVerticesHB(WGraphHB &g)
{
  return g.num_nodes();
}

static
void builtin_swapVectors(Vector<int32_t> &a, Vector<int32_t> &b)
{
  int n = a.getLength();
  if(n != b.getLength())
    throw std::runtime_error("vectors are not equal in length.");
  int * hosta = new int[n];
  int * hostb = new int[n];
  a.copyToHost(hosta, n);
  b.copyToHost(hostb, n);
  a.copyToDevice(hostb, n);
  b.copyToDevice(hosta, n);
  delete[] hosta;
  delete[] hostb;
}

template<typename T>
Vector<T> getBucketWithGraphItVertexSubset(BucketPriorityQueue<T> &pq){
    return pq.popDenseReadyVertexSet();
}

template<typename T>
void updateBucketWithGraphItVertexSubset(Vector<T> &vset, BucketPriorityQueue<T> &pq)
{
  pq.updateWithDenseVertexSet(vset);
}

static
void deleteObject(Vector<int32_t> &a) {
  a.exit();
}
}
