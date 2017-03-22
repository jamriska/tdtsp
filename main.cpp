// Entry for the kiwi.com Traveling Salesman Challenge (https://travellingsalesman.cz)
// Authors: Ondrej Jamriska & Jenda Keller

// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy
// and modify this file as you see fit.

#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <chrono>
#include <utility>
#include <algorithm>

#include "jzq.h"

// tour is a sequence of city indexes in their visiting order,e.g.: [0,3,1,2,0]
typedef std::vector<int> Tour;

// the central data-structure is a 3-dimensional table called "flightCosts"
// it provides lookups of the form: cost = flightCosts(onDay,fromCity,toCity)
// when the desired flight does not exist, the returned cost is -1
Array3<int> flightCosts;

int numCities;
int startCity;
std::vector<std::string> cityNames;

Tour globalBestTour;
int globalBestCost;

std::chrono::steady_clock::time_point timeStart;
double timeOut = 29.9;

const int COST_MAX = 32767500; // (500*65535)

struct CityCost
{
  int city;
  int cost;

  CityCost(int city,int cost):city(city),cost(cost) {}
  bool operator<(const CityCost& other) const { return cost < other.cost; }
};

double elapsedTime(std::chrono::steady_clock::time_point timeStart)
{
  return double(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-timeStart).count())/1000.0;
}

int evalTourCost(const Tour& tour,const Array3<int>& flightCosts) // returns -1 when the tour is not valid
{
  int tourCost = 0;
  for(int day=0;day<tour.size()-1;day++)
  {
    const int cost = flightCosts(day,tour[day],tour[day+1]);
    if(cost>0) { tourCost += cost; } else { return -1; }
  }
  return tourCost;
}

void printTour(FILE* file,const Tour& tour,const Array3<int>& flightCosts,const std::vector<std::string>& cityNames)
{
  fprintf(file,"%d\n",evalTourCost(tour,flightCosts));
  for(int day=0;day<tour.size()-1;day++)
  {
    fprintf(file,"%s %s %d %d\n",cityNames[tour[day]].c_str(),cityNames[tour[day+1]].c_str(),day,flightCosts(day,tour[day],tour[day+1]));
  }
}

void checkTimeOut()
{
  if(elapsedTime(timeStart)>=timeOut)
  {
    if(!globalBestTour.empty()) { printTour(stdout,globalBestTour,flightCosts,cityNames); }

    exit(0);
  }
}

Tour makeRandomTour(const int startCity,const int numCities,const Array3<int>& flightCosts,int maxIters=1000)
{
  for(int iter=0;iter<maxIters;iter++)
  {
    std::vector<int> citiesToVisit(numCities);
    for(int i=0;i<numCities;i++) { citiesToVisit[i] = i; }

    Tour tour;
    tour.push_back(startCity);
    citiesToVisit.erase(citiesToVisit.begin()+startCity);

    int currCity = startCity;
    for(int day=0;day<numCities;day++)
    {
      if(day==numCities-1) // on the last day we have to go back to the city we started from
      {
        if(flightCosts(day,currCity,startCity)>0)
        {
          tour.push_back(startCity);
          return tour;
        }
      }
      else // otherwise, go to a random city that we haven't visited yet
      {
        std::vector<int> reachableCities;
        for(int i=0;i<citiesToVisit.size();i++) // which un-visited cities are reachable from the current day/place?
        {
          if(flightCosts(day,currCity,citiesToVisit[i])>0)
          {
            reachableCities.push_back(citiesToVisit[i]);
          }
        }

        if(reachableCities.size()>0)
        {
          const int nextCity = reachableCities[rand()%reachableCities.size()]; // pick a random city that is reachable

          tour.push_back(nextCity);

          for(int i=0;i<citiesToVisit.size();i++) // remove it from the list of un-visited cities
          {
            if(citiesToVisit[i]==nextCity) { citiesToVisit.erase(citiesToVisit.begin()+i); break; }
          }

          currCity = nextCity;
        }
      }
    }
  }

  return Tour();
}

Array2<std::vector<CityCost>> sortOutboundFlights(const Array3<int>& flightCosts,const int numCities)
{
  Array2<std::vector<CityCost>> outboundFlights(numCities,numCities);

  for(int day=0;day<numCities;day++)
  for(int fromCity=0;fromCity<numCities;fromCity++)
  {
    outboundFlights(fromCity,day).reserve(numCities);

    for(int toCity=0;toCity<numCities;toCity++)
    {
      const int flightCost = flightCosts(day,fromCity,toCity);
      if(flightCost>0) { outboundFlights(fromCity,day).push_back(CityCost(toCity,flightCost));}
    }
  }

  for(int day=0;day<numCities;day++)
  for(int city=0;city<numCities;city++)
  {
    std::sort(outboundFlights(city,day).begin(),outboundFlights(city,day).end());
  }

  return outboundFlights;
}

Array2<std::vector<CityCost>> sortInboundFlights(const Array3<int>& flightCosts,const int numCities)
{
  Array2<std::vector<CityCost>> inboundFlights(numCities,numCities);

  for(int day=0;day<numCities;day++)
  for(int toCity=0;toCity<numCities;toCity++)
  {
    inboundFlights(toCity,day).reserve(numCities);

    for(int fromCity=0;fromCity<numCities;fromCity++)
    {
      const int flightCost = flightCosts(day,fromCity,toCity);
      if(flightCost>0) { inboundFlights(toCity,day).push_back(CityCost(fromCity,flightCost)); }
    }
  }

  for(int day=0;day<numCities;day++)
  for(int city=0;city<numCities;city++)
  {
    std::sort(inboundFlights(city,day).begin(),inboundFlights(city,day).end());
  }

  return inboundFlights;
}

Tour makeDoubleEndedNNTour(const int fromCity,
                           const int fromDay,
                           const int startCity,
                           const int numCities,
                           const Array3<int>& flightCosts,
                           const Array2<std::vector<CityCost>>& sortedOutboundFlights,
                           const Array2<std::vector<CityCost>>& sortedInboundFlights)
{
  std::vector<int> citiesToVisit(numCities,1);
  Tour tour(numCities+1);
  tour[fromDay] = fromCity;
  tour[numCities] = startCity;
  tour[0] = startCity;

  citiesToVisit[startCity] = 0;
  citiesToVisit[fromCity] = 0;

  int currTourEndDay = fromDay;
  int currTourStartDay = fromDay-1;
  int currTourEndCity = fromCity;
  int currTourStartCity = fromCity;

  while(1)
  {
    int bestNextCity = -1;
    int bestOutCost = COST_MAX;
    int bestPrevCity = -1;
    int bestInCost = COST_MAX;

    if(currTourEndDay==numCities-1) // on the last day we have to go back to the city we started from
    {
      if(flightCosts(currTourEndDay,currTourEndCity,startCity)<0)
      {
        return Tour(); // no flight to the start city on the last day was found
      }
    }
    else
    {
      const std::vector<CityCost>& outFlights = sortedOutboundFlights(currTourEndCity,currTourEndDay);
      for(int i=0;i<outFlights.size();i++)
      {
        const int nextCity = outFlights[i].city;
        if(citiesToVisit[nextCity])
        {
          const int outCost = outFlights[i].cost;
          if(outCost<bestOutCost)
          {
            bestNextCity = nextCity;
            bestOutCost = outCost;
            break;
          }
        }
      }
    }

    if(currTourStartDay==0) // on the first day we have to start from the startCity
    {
      if(flightCosts(0,startCity,currTourStartCity)<0)
      {
        return Tour(); // no flight from the start city on the first day was found
      }
    }
    else
    {
      const std::vector<CityCost>& inFlights = sortedInboundFlights(currTourStartCity,currTourStartDay);
      for(int i=0;i<inFlights.size();i++)
      {
        const int prevCity = inFlights[i].city;
        if(citiesToVisit[prevCity])
        {
          const int inCost = inFlights[i].cost;
          if(inCost<bestInCost)
          {
            bestPrevCity = prevCity;
            bestInCost = inCost;
            break;
          }
        }
      }
    }

    if(currTourEndDay==numCities-1 && currTourStartDay==0) { return tour; } // the tour is complete

    if(bestNextCity<0 && bestPrevCity<0) { return Tour(); } // no flights to currTourStartCity nor currTourEndCity were found

    if(bestOutCost<bestInCost)
    {
      currTourEndDay++;
      currTourEndCity = bestNextCity;
      citiesToVisit[currTourEndCity] = 0;
      tour[currTourEndDay] = currTourEndCity;
    }
    else
    {
      currTourStartCity = bestPrevCity;
      citiesToVisit[currTourStartCity] = 0;
      tour[currTourStartDay] = currTourStartCity;
      currTourStartDay--;
    }
  }

  return tour;
}

int evalGreedyNNTourCost(const int startDay,
                         const int numCities,
                         const int fromCity,
                         const int toCity,
                         const std::vector<int>& citiesNotVisitedYet,
                         const Array3<int>& flightCosts,
                         const Array2<std::vector<CityCost>>& sortedOutboundFlights)
{
  std::vector<int> citiesToVisit = citiesNotVisitedYet;
  citiesToVisit[fromCity] = 0;

  int flightsCost = 0;

  int currCity = fromCity;
  for(int day=startDay;day<numCities;day++)
  {
    bool connectionFound = false;

    if(day==numCities-1)
    {
      if(flightCosts(day,currCity,toCity) > 0)
      {
        flightsCost += flightCosts(day,currCity,toCity);
        connectionFound = true;
      }
    }
    else
    {
      const std::vector<CityCost>& outFlights = sortedOutboundFlights(currCity,day);
      for(int i=0;i<outFlights.size();i++)
      {
        const int nextCity = outFlights[i].city;
        if(citiesToVisit[nextCity])
        {
          citiesToVisit[nextCity] = 0;
          flightsCost += outFlights[i].cost;
          currCity = nextCity;
          connectionFound = true;
          break;
        }
      }
    }
    if(!connectionFound) { return COST_MAX; }
  }

  return flightsCost;
}

Tour makeNNTourWithLookAhead(const int startCity,
                             const int numCities,
                             const Array3<int>& flightCosts,
                             const Array2<std::vector<CityCost>>& sortedOutboundFlights)
{
  std::vector<int> citiesToVisit(numCities,1);

  Tour tour;
  tour.push_back(startCity);
  citiesToVisit[startCity] = 0;

  int currCity = startCity;
  for(int day=0;day<numCities;day++)
  {
    bool connectionFound = false;

    if(day==numCities-1) // on the last day we have to go back to the city we started from
    {
      if(flightCosts(day,currCity,startCity) > 0)
      {
        tour.push_back(startCity);
        connectionFound = true;
      }
    }
    else // otherwise, go to a city of the cheapest flight
    {
      const std::vector<CityCost>& outFlights = sortedOutboundFlights(currCity,day);
      int bestNextCity = -1;
      int bestTotalCost = COST_MAX;
      for(int i=0;i<outFlights.size();i++)
      {
        const int nextCity = outFlights[i].city;
        if(citiesToVisit[nextCity])
        {
          const int totalCost = outFlights[i].cost + evalGreedyNNTourCost(day+1,numCities,nextCity,startCity,citiesToVisit,flightCosts,sortedOutboundFlights);
          if(totalCost<bestTotalCost)
          {
            bestNextCity = nextCity;
            bestTotalCost = totalCost;
          }
        }
      }
      if(bestNextCity!=-1)
      {
        citiesToVisit[bestNextCity] = 0;
        tour.push_back(bestNextCity);
        currCity = bestNextCity;
        connectionFound = true;
      }
    }

    if(!connectionFound) { return Tour(); }
  }

  return tour;
}

Tour doubleBridge(const Tour& tour,const int day1,const int day2,const int day3,const int day4)
{
  Tour newTour;
  for(int i=0;i<day1;i++) { newTour.push_back(tour[i]); }
  for(int i=day3;i<day4;i++) { newTour.push_back(tour[i]); }
  for(int i=day2;i<day3;i++) { newTour.push_back(tour[i]); }
  for(int i=day1;i<day2;i++) { newTour.push_back(tour[i]); }
  for(int i=day4;i<tour.size();i++) { newTour.push_back(tour[i]); }
  return newTour;
}

Tour restrictedDoubleBridgeKick(const Tour& tour,const Array3<int>& flightCosts,const double maxAllowedCostIncrease,const int maxIters=100)
{
  const int originalCost = evalTourCost(tour,flightCosts);

  for(int iter=0;iter<maxIters;iter++) // keep generating double-bridge moves until we find a valid one
  {
    int days[4];

    while(1)
    {
      for(int i=0;i<4;i++) // generate a 4-tuple of random non-repeating days
      {
      retry:
        const int d = 1+(rand()%(tour.size()-1));
        for(int j=0;j<i;j++) if(days[j]==d) { goto retry; }
        days[i] = d;
      }

      std::sort(std::begin(days),std::end(days));

      // there should be at least one extra day between each two consecutive days
      if(days[1]>days[0]+1 &&
          days[2]>days[1]+1 &&
          days[3]>days[2]+1)
      {
        break; // accept the 4-tuple when this is satisfied. otherwise, generate a new one
      }
    }

    const Tour newTour = doubleBridge(tour,days[0],days[1],days[2],days[3]);
    const int cost = evalTourCost(newTour,flightCosts);
    if(cost>0 && (cost<maxAllowedCostIncrease*originalCost))
    {
      return newTour;
    }
  }

  return Tour();
}

Tour perform2Opt(const Tour& initialTour,const Array3<int>& flightCosts)
{
  Tour bestTour = initialTour;
  int bestCost = evalTourCost(bestTour,flightCosts);

  while(1)
  {
  from_scratch:
    checkTimeOut();

    for(int day1=1;day1<bestTour.size()-2;day1++)
    {
      Tour tour = bestTour;

      const int city1 = bestTour[day1];
      const int costFromTo1 = flightCosts(day1-1,bestTour[day1-1],city1)+
                              flightCosts(day1  ,city1,bestTour[day1+1]);

      for(int day2=day1+1;day2<bestTour.size()-1;day2++)
      {
        // swap
        if(day2==day1+1)
        {
          std::swap(bestTour[day1],bestTour[day2]);
          int cost = evalTourCost(bestTour,flightCosts);
          if(cost>0 && cost<bestCost)
          {
            bestCost = cost;
            goto from_scratch;
          }
          std::swap(bestTour[day1],bestTour[day2]);
        }
        else
        {
          const int city2 = bestTour[day2];

          if(flightCosts(day1-1,bestTour[day1-1],city2) > 0 &&
             flightCosts(day1  ,city2,bestTour[day1+1]) > 0 &&
             flightCosts(day2-1,bestTour[day2-1],city1) > 0 &&
             flightCosts(day2  ,city1,bestTour[day2+1]) > 0)
          {
            const int cost = bestCost-(costFromTo1+
                                       flightCosts(day2-1,bestTour[day2-1],city2)+
                                       flightCosts(day2  ,city2,bestTour[day2+1]))
                                     +(flightCosts(day1-1,bestTour[day1-1],city2)+
                                       flightCosts(day1  ,city2,bestTour[day1+1])+
                                       flightCosts(day2-1,bestTour[day2-1],city1)+
                                       flightCosts(day2  ,city1,bestTour[day2+1]));

            if(cost<bestCost)
            {
              std::swap(bestTour[day1],bestTour[day2]);
              bestCost = cost;
              goto from_scratch;
            }
          }
        }

        // flip
        {
          for(int i=0;i<day2-day1+1;i++)
          {
            tour[day1+i] = bestTour[day2-i];
          }

          const int cost = evalTourCost(tour,flightCosts);

          if(cost>0 && cost<bestCost)
          {
            bestTour = tour;
            bestCost = cost;
            goto from_scratch;
          }
        }
      }
    }

    break; // exhausted all improving moves, terminate
  }

  return bestTour; // the tour is now 2-opt
}

void updateDontLookBits(const Tour& oldTour,const Tour& newTour,std::vector<int>* inout_dontLookBits)
{
  std::vector<int>& dontLookBits = *inout_dontLookBits;
  const int numCities = oldTour.size()-1;
  const int resetDepth = 3;
  for(int city=0;city<numCities;city++)
  {
    int oldTourSlot = -1;
    int newTourSlot = -1;
    for(int slot=0;slot<oldTour.size();slot++) { if(oldTour[slot]==city) { oldTourSlot = slot; break; }}
    for(int slot=0;slot<newTour.size();slot++) { if(newTour[slot]==city) { newTourSlot = slot; break; }}

    if((oldTourSlot>0                && newTourSlot>0                && oldTour[oldTourSlot-1]!=newTour[newTourSlot-1]) ||
       (oldTourSlot<oldTour.size()-1 && newTourSlot<newTour.size()-1 && oldTour[oldTourSlot+1]!=newTour[newTourSlot+1]))
    {
      for(int offset=-resetDepth;offset<=+resetDepth;offset++)
      {
        const int target = oldTourSlot+offset;
        if(target>=0 && target<newTour.size())
        {
          dontLookBits[oldTour[target]] = 0;
        }
      }
    }
  }
}

Tour perform2OptWithDLBs(const Tour& initialTour,const Array3<int>& flightCosts)
{
  Tour bestTour = initialTour;
  int bestCost = evalTourCost(bestTour,flightCosts);

  const int numCities = flightCosts.width();
  std::vector<int> dontLookBits = std::vector<int>(numCities,0);

  while(1)
  {
  from_scratch:
    checkTimeOut();

    for(int day1=1;day1<bestTour.size()-2;day1++)
    {
      if(dontLookBits[bestTour[day1-1]]==1) { continue; }

      Tour tour = bestTour;

      const int city1 = bestTour[day1];
      const int costFromTo1 = flightCosts(day1-1,bestTour[day1-1],city1)+
                              flightCosts(day1  ,city1,bestTour[day1+1]);

      for(int day2=day1+1;day2<bestTour.size()-1;day2++)
      {
        // swap
        if(day2==day1+1)
        {
          Tour tour = bestTour;
          std::swap(tour[day1],tour[day2]);
          int cost = evalTourCost(tour,flightCosts);
          if(cost>0 && cost<bestCost)
          {
            updateDontLookBits(bestTour,tour,&dontLookBits);
            bestTour = tour;
            bestCost = cost;
            goto from_scratch;
          }
        }
        else
        {
          const int city2 = bestTour[day2];

          if(flightCosts(day1-1,bestTour[day1-1],city2) > 0 &&
             flightCosts(day1  ,city2,bestTour[day1+1]) > 0 &&
             flightCosts(day2-1,bestTour[day2-1],city1) > 0 &&
             flightCosts(day2  ,city1,bestTour[day2+1]) > 0)
          {
            const int cost = bestCost-(costFromTo1+
                                       flightCosts(day2-1,bestTour[day2-1],city2)+
                                       flightCosts(day2  ,city2,bestTour[day2+1]))
                                     +(flightCosts(day1-1,bestTour[day1-1],city2)+
                                       flightCosts(day1  ,city2,bestTour[day1+1])+
                                       flightCosts(day2-1,bestTour[day2-1],city1)+
                                       flightCosts(day2  ,city1,bestTour[day2+1]));

            if(cost<bestCost)
            {
              Tour tour = bestTour;
              std::swap(tour[day1],tour[day2]);
              updateDontLookBits(bestTour,tour,&dontLookBits);
              bestTour = tour;
              bestCost = cost;
              goto from_scratch;
            }
          }
        }

        // flip
        {
          for(int i=0;i<day2-day1+1;i++)
          {
            tour[day1+i] = bestTour[day2-i];
          }

          const int cost = evalTourCost(tour,flightCosts);

          if(cost>0 && cost<bestCost)
          {
            updateDontLookBits(bestTour,tour,&dontLookBits);
            bestTour = tour;
            bestCost = cost;
            goto from_scratch;
          }
        }
      }

      dontLookBits[bestTour[day1-1]] = 1;
    }

    break; // exhausted all improving moves, terminate
  }

  return bestTour; // the tour is now 2-opt
}

int parseInt(char** input)
{
  char*& in = *input;
  int num = 0;

  while(*in>='0' && *in<='9')
  {
    num = 10*num + (*in-'0');
    in++;
  }

  return num;
}

bool readInputFast(FILE* file,int* out_numCities,int* out_startCity,Array3<int>* out_flightCost,std::vector<std::string>* out_cityNames)
{
  struct Cities
  {
    int                       count;
    std::vector<std::string>  names;
    Array3<int>               nameToIndex;

    Cities():count(0),nameToIndex(128,128,128)
    {
      for(int i=0;i<nameToIndex.numel();i++) { nameToIndex[i] = -1; }
    }

    int makeIndex(const char* name)
    {
      int& index = nameToIndex(name[0],name[1],name[2]);
      if(index==-1)
      {
        index = count;
        names.push_back(name);
        count++;
      }
      return index;
    }
  };

  struct Flight
  {
    int fromCity,toCity,day,cost;
    Flight(int fromCity,int toCity,int day,int cost) : fromCity(fromCity),toCity(toCity),day(day),cost(cost) {}
  };

  Cities cities;
  std::vector<Flight> flights;

  char line[4096];
  fgets(line,4095,file);

  const char startName[4] = { line[0],
                              line[1],
                              line[2],
                              '\0' };

  const int startCity = cities.makeIndex(startName);

  while(1)
  {
    if(fgets(line,4095,file)==NULL) { break; }

    const char fromCity[4] = { line[0],
                               line[1],
                               line[2],
                               '\0' };

    const char   toCity[4] = { line[4],
                               line[5],
                               line[6],
                               '\0' };

    char* input = &line[8];
    const int day  = parseInt(&input);
    input++;
    const int cost = parseInt(&input);

    flights.push_back(Flight(cities.makeIndex(fromCity),cities.makeIndex(toCity),day,cost));
  }

  const int numCities = cities.count;

  Array3<int> flightCosts(numCities,numCities,numCities);
  for(int i=0;i<flightCosts.numel();i++) { flightCosts[i] = -1; }

  for(int i=0;i<flights.size();i++)
  {
    const Flight& f = flights[i];
    flightCosts(f.day,f.fromCity,f.toCity) = f.cost;
  }

  if(out_numCities!=0)  { *out_numCities  = numCities; }
  if(out_startCity!=0)  { *out_startCity  = startCity; }
  if(out_flightCost!=0) { *out_flightCost = flightCosts; }
  if(out_cityNames!=0)  { *out_cityNames  = cities.names; }

  return true;
}

void solveBruteForce()
{
  Tour tour;
  tour.push_back(startCity);
  for(int i=0;i<numCities;i++) { if(i!=startCity) { tour.push_back(i); } }
  tour.push_back(startCity);

  globalBestCost = COST_MAX;

  while(1)
  {
    const int cost = evalTourCost(tour,flightCosts);

    if(cost>0 && cost<globalBestCost)
    {
      globalBestTour = tour;
      globalBestCost = cost;
    }

    if(!std::next_permutation(tour.begin()+1,tour.end()-1)) { break; }
  }

  if(!globalBestTour.empty()) { printTour(stdout,globalBestTour,flightCosts,cityNames); }

  exit(0);
}

int main(int argc,char** argv)
{
  timeStart = std::chrono::steady_clock::now();

  readInputFast(stdin,&numCities,&startCity,&flightCosts,&cityNames);

  if(numCities<=10) { solveBruteForce(); }

  const Array2<std::vector<CityCost>> sortedOutboundFlights = sortOutboundFlights(flightCosts,numCities);

  Tour initTour = makeNNTourWithLookAhead(startCity,numCities,flightCosts,sortedOutboundFlights);

  if(initTour.empty())
  {
    const Array2<std::vector<CityCost>> sortedInboundFlights = sortInboundFlights(flightCosts,numCities);

    Tour bestDENNTour;
    int bestDENNCost = COST_MAX;
    for(int iter=0;iter<1000;iter++)
    {
      const int fromCity = 1+(rand()%(numCities-1));
      const int fromDay  = 1+(rand()%(numCities-1));
      const Tour tour = makeDoubleEndedNNTour(fromCity,fromDay,startCity,numCities,flightCosts,sortedOutboundFlights,sortedInboundFlights);
      if(!tour.empty())
      {
        int cost = evalTourCost(tour,flightCosts);
        if(cost>0 && cost<bestDENNCost)
        {
          bestDENNCost = cost;
          bestDENNTour = tour;
        }
      }
    }

    initTour = bestDENNTour;
  }

  if(initTour.empty()) { initTour = makeRandomTour(startCity,numCities,flightCosts,10000); }

  if(initTour.empty()) { exit(0); }

  initTour = perform2Opt(initTour,flightCosts);

  const int initCost = evalTourCost(initTour,flightCosts);

  globalBestTour = initTour;
  globalBestCost = initCost;

  Tour tour = globalBestTour;
  int cost = globalBestCost;

  std::chrono::steady_clock::time_point timeOfLastImprovement = std::chrono::steady_clock::now();
  while(1)
  {
    checkTimeOut();

    if(numCities<100 && elapsedTime(timeOfLastImprovement)>4.0)
    {
      const Tour restartTour = restrictedDoubleBridgeKick(globalBestTour,flightCosts,1.15,2000);
      if(!restartTour.empty())
      {
        tour = restartTour;
        cost = evalTourCost(tour,flightCosts);
        timeOfLastImprovement = std::chrono::steady_clock::now();
      }
    }

    const double maxAllowedCostIncrease = (numCities<100) ? 1.35 : (numCities>100 ? 1.075 : 1.1);

    Tour kickTour = restrictedDoubleBridgeKick(tour,flightCosts,maxAllowedCostIncrease,2000);

    if(!kickTour.empty())
    {
      kickTour = perform2OptWithDLBs(kickTour,flightCosts);
      const int kickCost = evalTourCost(kickTour,flightCosts);

      if(kickCost<cost)
      {
        tour = kickTour;
        cost = kickCost;
        timeOfLastImprovement = std::chrono::steady_clock::now();
      }
    }

    if(cost<globalBestCost)
    {
      globalBestTour = tour;
      globalBestCost = cost;
    }
  }

  return 0;
}
