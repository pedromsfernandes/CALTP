#include <iostream>
#include <ctime>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include "elevation.h"
#include "createSharing.h"

using namespace std;

#define CURL_RESULT "curl_result.txt"

vector<string> readNodes(string file, vector<string> &ids)
{
    ifstream read(file);
    string line;
    vector<string> lines;

    if (!read.is_open())
    {
        cerr << "File " << file << " not found! Exiting..." << endl;
        exit(1);
    }

    while (true)
    {
        getline(read, line);
        ids.push_back(line.substr(0, line.find(";")));

        if (!read.eof())
            lines.push_back(line.substr(0, line.size() - 1));
        else
            break;
    }

    read.close();
    return lines;
}

void saveResults(const vector<string> &originalLines, const vector<double> &elevations, string file, const set<int> &sharingLines)
{
    ofstream save(file);

    int size = originalLines.size();

    for (int i = 0; i < size; i++)
    {
        save << originalLines.at(i) << ";" << elevations.at(i);

        if (sharingLines.find(i) != sharingLines.end())
            save << ";true";
        else
            save << ";false";

        if (i < size - 1)
            save << endl;
    }

    save.close();
}

int usage(char *argv[])
{
    cout << "Usage: " << argv[0] << " <nodes file> <output file> <sharingLocations file> <number of sharingLocations>" << endl;
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc != 5)
        return usage(argv);

    srand(time(NULL));

    int fd = open(CURL_RESULT, O_RDWR | O_CREAT | O_TRUNC, 0644);
    int save_stdout = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);

    vector<string> ids;
    vector<string> lines;
    string locations;
    vector<double> elevations;
    vector<double> temp_elevations;
    vector<string> sharingInfo;
    set<int> lineNumbers;
    int linesSize;

    lines = readNodes(argv[1], ids);
    linesSize = lines.size();

    int step = 2000;
    int begin = 0;
    int end = 0;
    int i = 1;
    while (end < linesSize)
    {
        end += step;
        locations = parseLocations(lines, begin, end);
        system(getCommand(locations).c_str());
        cout << endl;
        temp_elevations = parseResult(CURL_RESULT, i);
        elevations.insert(std::end(elevations), std::begin(temp_elevations), std::end(temp_elevations));
        begin = end;
        i++;
    }

    dup2(save_stdout, STDOUT_FILENO);
    close(save_stdout);
    close(fd);

    lineNumbers = generateRandomLineIDs(atoi(argv[4]), linesSize);
    sharingInfo = createSharingInfo(ids, lineNumbers);
    createSharingFile(argv[3], sharingInfo);
    saveResults(lines, elevations, argv[2], lineNumbers);

    cout << endl
         << endl
         << "Successfully added elevation and sharing location information to the specified files!" << endl;

    return 0;
}