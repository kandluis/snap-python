#include <time.h>
#include <cassert>
#include <cstdio>
#include <climits>

namespace TSnap {

typedef TVec<TInt, int> TIntV;
typedef TVec<TIntV, int> TIntIntVV;
typedef TVec<TIntIntVV, int> TIntVVV;

typedef THash<TInt, TVec< TInt, int> > TIntIntVH;
typedef THash<TInt, TIntIntVV> TIntVVH;

// vector of hashes
typedef TVec<THash<TInt,TInt>, int> TIntIntHV;


// TODO (smacke): It makes a whole lot of sense to define
// types for offset'd and segment'd containers

void SeedRandom() {
  long int ITime;
  long int IPid;
  long int RSeed;

  ITime = (long int) time(NULL);
  IPid = (long int) getpid();

  RSeed = ITime * IPid;
  srand48(RSeed);
}

void Randomize(TIntV& Vec) {
  int Pos;
  int Last = Vec.Len() - 1;
  for (int ValN = Last; ValN > 0; ValN--) {
    Pos = (long) (drand48() * ValN);
    Vec.Swap(ValN, Pos);
  }
}

int StdDist(double Mean, double Dev) {
  int i;
  double x;

  x = -6.0;
  for (i = 0; i < 12; i++) {
    x += drand48();
  }

  x *= Dev;
  x += Mean;

  return int(x + 0.5);
}

#if 0
void GetDegrees(TIntV* Nodes, double Mean, double Dev) {
  int i;
  int d;
  int Len;
  printf("GetDegrees\n");
  printf("Nodes Len %d\n",Nodes->Len());

  Len = Nodes->Len();
  for (i = 0; i < Len; i++) {
    d = StdDist(Mean, Dev);
    printf("degree1 %d %d\n", i, d);
    (*Nodes)[i] = d;
  }

  for (i = 0; i < Len; i++) {
    printf("degree2 %d %d\n", i, Nodes->GetVal(i).Val);
  }
}
#endif


int bitcount(unsigned int n) {
  int v=0;
  while (n) {
    n &= (n-1);
    v++;
  }
  return v;
}

unsigned int nextPowerOf2(unsigned int v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

int leading(int64 val, int seg_bits) {
  return val >> seg_bits;
}

int trailing(int64 val, int seg_bits) {
  return int(val & ((1<<seg_bits)-1));
}

int64 zeroLowOrderBits(int64 val, int seg_bits) {
  return leading(val, seg_bits) << int64(seg_bits);
}

/*
 * Generate random 64 bit integer in half-open interval [0,range)
 */
int64 randRange64(int64 range, int seg_bits) {
  // idea: spit range into two parts (by segment),
  // and find random inclusive values for both halves.
  // retry if too large.
  int64 ret = range;
  while (ret == range) { // while too large (extremely improbable this will happen more than once)
    int64 left = int64(drand48() * (leading(range,seg_bits) + 1));
    int64 right = int64(drand48() * (trailing(range,seg_bits) + 1));
    ret = (left << int64(seg_bits)) + right;
  }
  return ret;
}

template<typename T>
void ensureCapacity(TVec<T> &vec, unsigned int size) {
  size++; // make sure we will actually be able to index vec at size
  int cap = vec.Reserved();

  // if this is the first time we add something to this entry
  // then reserve some memory
  if (vec.Len()==0) {
    unsigned int newSize = nextPowerOf2(size);
    cap = max(newSize, cap);
    vec.Reserve(newSize, cap);
  } else {
    // otherwise, check to see if we still don't have enough space
    // if not, make more
    unsigned int x = vec.Len();
    if (x < size) {
      while (x < size) x *= 2;
      cap = max(x, cap);
      vec.Reserve(x, cap);
    }
  }
}

// helper function for debugging
// converts a segmented vector into a nonsegmented one
TIntV desegment(TIntIntVV& segmented, int seg_bits) {
  TIntV desegmented;
  for (int64 seg=0; seg<segmented.Len(); seg++) {
    for (TIntV::TIter it = segmented[seg].BegI(); it != segmented[seg].EndI(); ++it) {
      desegmented.Add((seg<<seg_bits) + *it);
    }
  }
  return desegmented;
}

// helper function for debugging
// given a TIntVVV created by AssignRandomEdges64, it creates the unsegmented equivalent
TIntIntVV desegmentRandomizedEdges(TIntVVV& segmented, int seg_bits, int tsize) {
  TIntIntVV desegmented(segmented.Len());
  for (int i=0; i<segmented.Len(); i++) {
    TIntV onetask;
    int task_leading = leading(i*tsize,seg_bits) << seg_bits;
    for (int seg=0; seg<segmented[i].Len(); seg++) {
      for (int j=0; j<segmented[i][seg].Len(); j+=2) {
        onetask.Add(task_leading + segmented[i][seg][j]);
        onetask.Add((seg<<seg_bits) + segmented[i][seg][j+1]);
      }
    }
    desegmented[i] = onetask;
  }
  return desegmented;
}

TIntIntVV segment(TIntV& unsegmented, int seg_bits) {
  TIntIntVV segmented;
  for (TIntV::TIter it = unsegmented.BegI(); it != unsegmented.EndI(); ++it) {
    ensureCapacity(segmented, leading(*it, seg_bits));
    segmented[leading(*it, seg_bits)].Add(trailing(*it, seg_bits));
  }
  return segmented;
}

void FillVec(TIntV &Vec, int val) {
  // set all values to "val"
  for (TIntV::TIter it = Vec.BegI(); it != Vec.EndI(); ++it) {
    *it = val;
  }
}

void ZeroVec(TIntV& Nodes) {
  // set all values to zero
  FillVec(Nodes, 0);
}

void GetDegrees(TIntV& Nodes, double Mean, double Dev) {
  int d;
  //printf("GetDegrees\n");
  //printf("Nodes Len %d\n",Nodes.Len());

  // assign degree to each node
  for (TIntV::TIter i = Nodes.BegI(); i != Nodes.EndI(); i++) {
    d = StdDist(Mean, Dev);
    //printf("degree1 %d %d\n", i, d);
    *i = d;
  }

  //for (i = 0; i < Len; i++) {
  //printf("degree2 %d %d\n", i, Nodes[i].Val);
  //}
}

void IncVal(TIntV& Nodes, int disp) {
  // increment value for each element
  for (TIntV::TIter i = Nodes.BegI(); i != Nodes.EndI(); i++) {
    *i += disp;
  }
}

int64 GetMemSize64(TIntIntVV& Vec64) {
  int64 total=2*sizeof(int);
  for (int i=0; i<Vec64.Len(); i++) {
  total += Vec64.GetVal(i).GetMemSize();
  }
  return total;
}


void AssignRndTask(const TIntV& Nodes, TIntIntVV& Tasks) {
  int i;
  int j;
  int n;
  int t;
  int NumNodes;
  int NumTasks;
  //printf("AssignRndTask\n");
  //printf("Nodes Len %d\n",Nodes.Len());
  //printf("Tasks Len %d\n",Tasks.Len());

  NumNodes = Nodes.Len();
  NumTasks = Tasks.Len();

  // distribute stubs randomly to tasks
  for (i = 0; i < NumNodes; i++) {
    n = Nodes[i].Val;
    //printf("degree3 %d %d\n", i, n);
    for (j = 0; j < n; j++) {
      t = (long) (drand48() * NumTasks);
      Tasks[t].Add(i);
    }
  }
}


/*
 * Given knowledge of the degree of each node in the graph, offset from "base",
 * this function generates stubs and assigns them to tasks randomly.
 */
void AssignRndTask64(const TIntV &NodeDegrees, TIntVVV &Tasks, const int64 base, const int seg_bits) {

//  assert(NodeDegrees.Len() % seg_bits == 0);

  // distribute stubs randomly to tasks
  for (int i = 0; i < NodeDegrees.Len(); i++) {
    int degree = NodeDegrees[i].Val;
    int64 nodeId = base + i;
    int high_order = leading(nodeId, seg_bits);
//    printf("add %d of %ld to %d tasks\n", degree, nodeId, Tasks.Len());
    for (int j = 0; j < degree; j++) {
      int t = (long) (drand48() * Tasks.Len());
      // make sure this TIntIntVV has enough space
      ensureCapacity(Tasks[t], high_order);
      Tasks[t][high_order].Add(trailing(nodeId, seg_bits));
    }
  }
}

void AddVec64(TIntIntVV &vecA, const TIntIntVV &vecB) {
  ensureCapacity(vecA, vecB.Len()-1);
  for (int i=0; i<vecB.Len(); i++) {
    vecA[i].AddV(vecB[i]);
  }
}

/**
 * samples from a ragged table, using some extra structures for bookkeeping
 */
int64 GetRandomStub64(TIntIntVV &stubs, TIntV &stubsRemainingInSegment, int64 &totalStubsRemaining, int seg_bits) {
  assert(totalStubsRemaining > 0);
  int64 randomStubIndex;

  // if we need a ton of random bits
  if (totalStubsRemaining > INT_MAX) {
    // split random calc into low order and high order portions
    randomStubIndex = randRange64(totalStubsRemaining, seg_bits);
  } else {
    randomStubIndex = int64(drand48() * totalStubsRemaining);
  }

  totalStubsRemaining--;
  int64 stubsSeen=0;
  for (int64 i=0; i<stubs.Len(); i++) {
    int64 prevSeen = stubsSeen;
    stubsSeen += stubsRemainingInSegment[i].Val;
    // keep going until we exceed threshold -- at this point, we sample
    if (stubsSeen > randomStubIndex) {
      int stubIndexWithinSegment = (int)(randomStubIndex - prevSeen);
      int stub = stubs[i][stubIndexWithinSegment].Val;
      // now move this stub to the end so that it won't be selected again
      stubsRemainingInSegment[i]--;
      stubs[i].Swap(stubIndexWithinSegment, stubsRemainingInSegment[i].Val); // this isn't prone to off-by-one errors at all!
      return (i<<seg_bits) + stub;
    }
  }
  assert(false); // we should never get here
  return -1;
}


void AssignRandomEdges64(TIntIntVV &stubs, TIntVVV &Tasks, int tsize, int seg_bits) {
  TIntV stubsRemainingInSegment(stubs.Len());
  int64 totalStubsRemaining = 0;
  for (int i=0; i<stubs.Len(); i++) {
    stubsRemainingInSegment[i] = stubs[i].Len();
    totalStubsRemaining += stubs[i].Len();
  }
  while (totalStubsRemaining > 1) {
//    printf("%d remaining\n", totalStubsRemaining);
//    printf("about to get random stubs\n");
    int64 stubA = GetRandomStub64(stubs, stubsRemainingInSegment, totalStubsRemaining, seg_bits);
    int64 stubB = GetRandomStub64(stubs, stubsRemainingInSegment, totalStubsRemaining, seg_bits);
//    printf("done getting random stubs\n");

    int taskId = (int)(stubA / tsize);
    // make sure we have enough room there
//    printf("about to ensure capacity\n");
    ensureCapacity(Tasks[taskId], leading(stubB, seg_bits));
//    printf("done ensuring capacity\n");

    /*
     * This next part is kind of weird. We need to keep the two stubs together,
     * even though they could have very different high order bits. The thing is,
     * whichever task gets (stubA, stubB) should be able to infer the high-order
     * bits of stubA based on the id of the task, since certain tasks are only
     * responsible for certain neighbor lists. For this to work properly we need
     * the segment size to be a multiple of "range".
     */

//    printf("about to add stubs\n");
    Tasks[taskId][leading(stubB, seg_bits)].Add(trailing(stubA, seg_bits));
    Tasks[taskId][leading(stubB, seg_bits)].Add(trailing(stubB, seg_bits));
//    printf("done adding stubs\n");

    taskId = (int)(stubB / tsize);
    ensureCapacity(Tasks[taskId], leading(stubA, seg_bits));

    // again, the weird part
//    printf("about to add stubs round 2\n");
    Tasks[taskId][leading(stubA, seg_bits)].Add(trailing(stubB, seg_bits));
    Tasks[taskId][leading(stubA, seg_bits)].Add(trailing(stubA, seg_bits));
//    printf("done adding stubs round 2\n");
  }
}

void AssignEdges(const TIntV& Pairs, TIntIntVV& Tasks, int tsize) {
  int i;
  int NumStubs;
  int NumTasks;
  int TaskId;
  int Node1;
  int Node2;

  //printf("AssignEdges\n");
  //printf("Pairs Len %d\n",Pairs.Len());
  //printf("Tasks Len %d\n",Tasks.Len());

  NumStubs = Pairs.Len();
  NumTasks = Tasks.Len();

  // distribute edges to tasks
  for (i = 0; i < NumStubs-1; i += 2) {

    Node1 = Pairs.GetVal(i).Val;
    Node2 = Pairs.GetVal(i+1).Val;

    // add an edge twice, once for each end node
    TaskId = Node1 / tsize;
    Tasks[TaskId].Add(Node1);
    Tasks[TaskId].Add(Node2);

    TaskId = Node2 / tsize;
    Tasks[TaskId].Add(Node2);
    Tasks[TaskId].Add(Node1);
  }
}

void GetAdjLists(const TIntV& Edges, TIntIntVH& AdjLists) {
  int i;
  int NumStubs;
  int Node1;
  int Node2;

  //printf("GetAdjLists\n");
  //printf("Edges1 Len %d\n",Edges.Len());

  NumStubs = Edges.Len();

  // distribute node pairs to adjacency lists
  for (i = 0; i < NumStubs-1; i += 2) {
    Node1 = Edges.GetVal(i).Val;
    Node2 = Edges.GetVal(i+1).Val;

    AdjLists.AddKey(Node1);
    AdjLists(Node1).AddMerged(Node2);
  }
}

/*
 * Creates an adjacency list out of segmented edge table.
 * NOTE: the reason this works is that the leading bits of the
 *       first node in the edge (Node1) are determined by this task.
 */
void GetAdjLists64(const TIntIntVV &Edges, TIntVVH &AdjLists) {
  for (int seg=0; seg<Edges.Len(); seg++) {
    for (int j=0; j<Edges[seg].Len(); j+=2) {
      int Node1 = Edges[seg][j].Val;
      int Node2 = Edges[seg][j+1].Val;

      ensureCapacity(AdjLists(Node1), seg);

      AdjLists(Node1)[seg].AddMerged(Node2);
    }
  }
}

void GetNeighborhood(const TIntV& Nodes, const TIntIntVH& AdjLists, TIntV& Hood) {
  int i;
  int j;
  int Node;
  int NumNodes;
  int NumNeighbors;
  int Neighbor;
  TIntH HashHood;
  TIntV Neighbors;

  NumNodes = Nodes.Len();

  // create a union of all neighbors
  for (i = 0; i < NumNodes; i++) {
    Node = Nodes.GetVal(i).Val;
    Neighbors = AdjLists.GetDat(Node);
    NumNeighbors = Neighbors.Len();
    for (j = 0; j < NumNeighbors; j++) {
      Neighbor = Neighbors[j].Val;
      HashHood.AddDat(Neighbor,0);
    }
  }

  // change a hash table to a vector
  HashHood.GetKeyV(Hood);
}

void GetNeighborhood64(const TIntV &Nodes, const TIntVVH &AdjLists, TIntIntVV &Hood) {
  TIntIntHV HashHood;
  // not segmented since this task is responsible for
  // low-order bits of all incoming nodes
  for (int i=0; i<Nodes.Len(); i++) {
//    bool found=false;
    int Node = Nodes[i].Val;
    if (!AdjLists.IsKey(Node)) {
      printf("no neighbors for node %d\n", Node);
      continue; // don't seg fault if node has no neighbors
    }
    TIntIntVV Neighbors = AdjLists.GetDat(Node);
    ensureCapacity(HashHood, Neighbors.Len()-1);
    for (int seg=0; seg<Neighbors.Len(); seg++) {
      for (int j=0; j<Neighbors[seg].Len(); j++) {
        int Neighbor = Neighbors[seg][j].Val;
        HashHood[seg].AddDat(Neighbor,0); // we are using this as a set
//        found=true;
      }
    }
//    if (!found) {
//      printf("no neighbors for node %d after going through all segments!\n", Node);
//    }
  }

  ensureCapacity(Hood, HashHood.Len()-1);
  for (int seg=0; seg<HashHood.Len(); seg++) {
    HashHood[seg].GetKeyV(Hood[seg]);
  }
}

void Edge2Hash(const TIntV& Edges, TIntH& Hash) {
  int i;
  int Num;
  int Key;
  int Value;

  //printf("Edges2 Len %d\n",Edges.Len());
  Num = Edges.Len();

  for (i = 0; i < Num-1; i += 2) {
    Key = Edges.GetVal(i).Val;
    Value = Edges.GetVal(i+1).Val;

    Hash.AddDat(Key, Value);
  }
}

void GetNewNodes(const TIntV& Nodes, TIntH& Visited, TIntH& NewNodes, int distance) {
  int i;
  int Num;
  int Node;

  //printf("GetNewNodes Nodes %d\n",Nodes.Len());
  Num = Nodes.Len();

  for (i = 0; i < Num; i++) {
    Node = Nodes.GetVal(i).Val;

    if (!Visited.IsKey(Node)) {
      NewNodes.AddDat(Node,0);
      Visited.AddDat(Node,distance);
    }
  }

  //printf("GetNewNodes NewNodes %d\n",NewNodes.Len());
}

void GetNewNodes1(const TIntV& FringeNodes, TIntV& Visited, TIntV& NewNodes, int distance) {
  int Node;

  //printf("GetNewNodes1 Nodes %d\n",Nodes.Len());

  for (TIntV::TIter i = FringeNodes.BegI(); i != FringeNodes.EndI(); i++) {
    Node = *i;

    if (Visited[Node] <= 0) {
      Visited[Node] = distance;
      NewNodes.Add(Node);
    } else {
      assert(Visited[Node] <= distance);
    }
  }

  //printf("GetNewNodes1 NewNodes %d\n",NewNodes.Len());
}

void GetDistances(const TIntV& Visited, TIntV& DistCount) {
  for (TIntV::TIter i = Visited.BegI(); i != Visited.EndI(); i++) {
    DistCount[*i]++;
  }
}

void Nodes2Tasks(const TIntH& Nodes, TIntIntVV& Tasks, int tsize) {
  int Node;
  int TaskId;

  for (TIntH::TIter It = Nodes.BegI(); It < Nodes.EndI(); It++) {
    Node = It.GetKey();
    TaskId = Node / tsize;

    //printf("Nodes2Tasks node %d, task %d\n", Node, TaskId);
    Tasks[TaskId].Add(Node);
  }
}

void Nodes2Tasks1(const TIntV& Nodes, TIntIntVV& Tasks, int tsize) {
  int Node;
  int TaskId;

  for (TIntV::TIter i = Nodes.BegI(); i != Nodes.EndI(); i++) {
    Node = *i;
    TaskId = Node / tsize;

    //printf("Nodes2Tasks node %d, task %d\n", Node, TaskId);
    Tasks[TaskId].Add(Node);
  }
}

void Nodes2Tasks64(const TIntIntVV &Nodes, TIntIntVV &Tasks, int tsize, int seg_bits) {
  for (int seg=0; seg<Nodes.Len(); seg++) {
    int64 base = ((int64)seg) << seg_bits;
    for (int node_i=0; node_i<Nodes[seg].Len(); node_i++) {
      int node = Nodes[seg][node_i];
      int TaskId = int(((base + node) / tsize));
      Tasks[TaskId].Add(node);
    }
  }
}

}; // namespace TSnap
