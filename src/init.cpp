#include "SystemManager.h"
#include "Utils.h"
#include <string>
#include <fstream>
#include <ctime>

using namespace std;

SystemManager::SystemManager()
{
	initGraphViewer();
}

void SystemManager::initGraphViewer()
{
	gv = new GraphViewer(WINDOW_WIDTH, WINDOW_HEIGHT, false);
	gv->defineEdgeColor(EDGE_COLOR_DEFAULT);
	gv->defineVertexColor(VERTEX_COLOR_DEFAULT);
}

void SystemManager::initFileNames(string nodes, string edges, string names, string sharing)
{
	this->fileNames.nodes = nodes;
	this->fileNames.edges = edges;
	this->fileNames.names = names;
	this->fileNames.sharingLocations = sharing;
}
void SystemManager::loadSharingLocations(vector<SharingLoc> &sharingLocations)
{
	ifstream file(fileNames.sharingLocations);

	cout << " - File: " << fileNames.sharingLocations << endl;

	if (!file.is_open())
	{
		cerr << endl << " - File not found!" << endl;
		exit(1);
	}

	unsigned long long id;
	char ign;
	int lotation, slots;

	while (!file.eof())
	{
		file >> id >> ign >> lotation >> ign >> slots;

		sharingLocations.push_back(SharingLoc(id, lotation, slots));
	}

	file.close();
}

void SystemManager::loadNodes(unordered_map<int, unsigned long long> &idsNodes, const vector<SharingLoc> &sharingLocations)
{
	ifstream read(fileNames.nodes);

	int idInt = 0;

	cout << " - File: " << fileNames.nodes << endl;

	if (!read.is_open())
	{
		cerr << endl << " - File not found!" << endl;
		exit(1);
	}
	else
	{
		while (!read.eof())
		{
			idInt++;
			unsigned long long id;

			double lat, lon, projx, projy, alt;
			char ign;
			string isShrLoc;

			read >> id >> ign >> lat >> ign >> lon >> ign >> projx >> ign >> projy >> ign >> alt >> ign;
			getline(read, isShrLoc);

			gv->addNode(idInt, (int)XCONST * convertLongitudeToX(lon), (int)YCONST * convertLatitudeToY(lat) - (int)YB);
			gv->setVertexLabel(idInt, to_string(idInt));

			idsNodes.insert(make_pair(idInt, id));

			if (isShrLoc == "true")
			{
				gv->setVertexColor(idInt, RED);
				for (auto sl : sharingLocations)
				{
					if (sl.id == id)
					{
						graph.addVertex(new SharingLocation(idInt, lat, lon, alt, sl.lotation, sl.slots));
						break;
					}
				}
			}
			else
			{
				graph.addVertex(new Location(idInt, lat, lon, alt));
			}
		}

		read.close();
	}
}

vector<EdgeName> SystemManager::loadNames()
{
	ifstream read(fileNames.names);
	cout << " - File: " << fileNames.names << endl;

	vector<EdgeName> edges;

	if (!read.is_open())
	{
		cerr << endl << " - File not found!" << endl;
		exit(1);
	}
	else
	{
		while (!read.eof())
		{
			unsigned long long id;
			string name, isBidirectional;
			char ign;

			read >> id >> ign;

			getline(read, name, ';');
			getline(read, isBidirectional);

			edges.push_back(EdgeName(id, name, isBidirectional == "True" ? true : false));
		}
		read.close();
	}

	return edges;
}

void SystemManager::loadEdges(vector<EdgeName> &edges, unordered_map<int, unsigned long long> &idsNode)
{
	ifstream read(fileNames.edges);

	cout << " - File: " << fileNames.edges << endl;

	unsigned long long id = -1, ori, dest;
	char ign;

	if (!read.is_open())
	{
		cerr << endl << " - File not found!" << endl;
		exit(1);
	}
	else
	{
		int idIntEdge = 0;
		while (!read.eof())
		{
			idIntEdge++;
			read >> id >> ign >> ori >> ign >> dest >> ign;

			Vertex *origin;
			Vertex *destiny;
			int origemID = 0, destinoID = 0;

			auto itOrigin = find_if(idsNode.begin(), idsNode.end(), [ori](auto inf) {
				return inf.second == ori;
			});

			origin = graph.findVertex(new Location(itOrigin->first));
			origemID = itOrigin->first;

			auto itDest = find_if(idsNode.begin(), idsNode.end(), [dest](auto inf) {
				return inf.second == dest;
			});

			destiny = graph.findVertex(new Location(itDest->first));
			destinoID = itDest->first;

			if (destiny == nullptr || origin == nullptr)
			{
				cerr << "\nError on Edges file. Some Location(node) is not defined. File could have been corrupted." << endl
					<< endl;
				exit(4);
			}

			double weight = calcWeight((origin->getInfo()), (destiny->getInfo()));

			auto x = find_if(edges.begin(), edges.end(), [id](EdgeName &edge) {
				return edge.id == id;
			});

			if (x != edges.end())
			{
				if (x->isBidirectional)
				{
					gv->addEdge(idIntEdge, origemID, destinoID, EdgeType::UNDIRECTED);
					graph.addEdge(origin->getInfo(), destiny->getInfo(), weight, idIntEdge, x->name);
					gv->setEdgeLabel(idIntEdge, x->name);
					idIntEdge++;
					weight = calcWeight(destiny->getInfo(), origin->getInfo());
					graph.addEdge(destiny->getInfo(), origin->getInfo(), weight, idIntEdge, x->name);
				}
				else
				{
					gv->addEdge(idIntEdge, origemID, destinoID, EdgeType::DIRECTED);
					graph.addEdge(origin->getInfo(), destiny->getInfo(), weight, idIntEdge, x->name);
					gv->setEdgeLabel(idIntEdge, x->name);
				}
			}
		}

		read.close();
	}

	if (id == -1)
	{
		cerr << "Graph with 0 elements." << endl;
		exit(2);
	}
}

unordered_map<int, unsigned long long> SystemManager::loadFiles()
{
	clock_t begin, end;
	double timeSpent;
	vector<SharingLoc> sharingLocations;
	unordered_map<int, unsigned long long> idsNodes;
	vector<EdgeName> edgesNames;

	begin = clock();
	loadSharingLocations(sharingLocations);
	end = clock();
	timeSpent = timeDiff(begin, end);
	cout << "         Time to read Sharing Locations file: " << timeSpent << " seconds" << endl << endl;

	begin = clock();
	loadNodes(idsNodes, sharingLocations);
	end = clock();
	timeSpent = timeDiff(begin, end);
	cout << "         Time to read Nodes file: " << timeSpent << " seconds" << endl << endl;

	begin = clock();
	edgesNames = loadNames();
	end = clock();
	timeSpent = timeDiff(begin, end);
	cout << "         Time to read Names file: " << timeSpent << " seconds" << endl << endl;

	begin = clock();
	loadEdges(edgesNames, idsNodes);
	end = clock();
	timeSpent = timeDiff(begin, end);
	cout << "         Time to read Edges file: " << timeSpent << " seconds" << endl;

	vector<Vertex*> vertexSet = graph.getVertexSet();
	for (unsigned int i = 0; i < vertexSet.size(); i++)
	{
		if (vertexSet.at(i)->getAdj().size() > 2 && vertexSet.at(i)->getInfo()->getColor() != RED)
		{
			gv->setVertexColor(i + 1, MAGENTA);
			graph.findVertex(new Location(i + 1))->getInfo()->setColor(MAGENTA);
		}
	}
	
	gv->rearrange();

	return idsNodes;
}