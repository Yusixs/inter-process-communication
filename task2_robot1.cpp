#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include <fstream>
#include <errno.h>

using namespace std;

int main() {


    // ================ CHANGEABLE VARIABLES SECTION (IMPORTANT) ================

    // Self-explanatory. If this is changed, please adjust identifiers and number of robot files 
    const int totalRobots = 4;
    
    // Identifier is the index we will assign this process for all the lists and its order in the read/write sequences below.
    // E.g Identifier = 2 would allocate this robot to 3rd position in all lists and 3rd position for broadcasting messages.
    const int identifier = 0;

    // ================ VARIABLES SECTION ================
    
    // Contains list of all robot ids within this process's proximity + the current robot's own id. Default to 0 for IDs which aren't in proximity 
    int RobotID[totalRobots];
    
    // Stored the coordinates of all the robots after the read/write sequences + the current robot's own coordinates
    int robotCoords[totalRobots][2];

    // FIFO Pipe names. They follow the naming format "Sender-Receiver". With the help of identifier
    const char* writeLabels[totalRobots]; 
    const char* readLabels[totalRobots]; 
    string generateWriteLabels[totalRobots];
    string generateReadLabels[totalRobots];

    // Standard fileDescriptor variable changed frequently
    int fileDescriptor;

    // Stores euclidean distances between current robot w.r.t other robots by using robotCoords variable
    int euclideans[totalRobots];

    // I can't pass 0 in write functions so this variable is acting as the middle man
    const int dontRead = 0;


    // ================ CODE SECTION ================

    // Initializing Pipe Names 
    // (E.g for identifier 2 and robot size 3 it will create)
    // [2-to-0, 2-to-1, 2-to-2, 2-to-3] for writeLabels
    // [0-to-2, 1-to-2, 2-to-2, 3-to-2] for readLabels
    for (int i = 0; i < totalRobots; i++){
        generateWriteLabels[i] = to_string(identifier) + string("-to-") + to_string(i);
        writeLabels[i] =  generateWriteLabels[i].data();
        generateReadLabels[i] = to_string(i) + string("-to-") + to_string(identifier);
        readLabels[i] =  generateReadLabels[i].data();
    }


    // Initialize robot locations randomly (Between 1 and 30 inclusive)
    for(int i = 0; i < totalRobots; i++)
        for(int j = 0; j < 2; j++)
            robotCoords[i][j] = 1 + (rand() % 30);


    // Initialize robot's own ID by using identifier and reading from file generated from task1
    ifstream rollNumberFile("rollnumbers.txt");
    if (rollNumberFile.is_open()) {
        for (int i = 0; i < totalRobots; i++) {
            int temp;
            if (i == identifier) 
                rollNumberFile >> RobotID[i];
            else
                rollNumberFile >> temp;
        }
        rollNumberFile.close();
    } else {
        cout << "Error in reading file name, maybe the file doesn't exist in the specified location?";
        return 0;
    }



    // Now input the robot locations from the user
    cout << "Hello, I am Robot #" << RobotID[identifier] << ". ";
    cout << "My current coordinates are (" << robotCoords[identifier][0] << "," << robotCoords[identifier][1] << "). ";
    cout << "Please enter my new coordinates: \n";


    // For X coordinate and then Y coordinate
    for(int i = 0; i < 2; i++) {
        // Keep taking input until input is between 1 and 30 inclusive
        do {
            string output = (i == 0) ? "\tx = " : "\ty = ";
            cout << output;
            cin >> robotCoords[identifier][i];
            while (!cin.good()) {
                cin.clear();
                cin.ignore(1024, '\n');
                cout << "Please enter a number!" << endl;
                cout << output;
                cin >> robotCoords[identifier][i];

            }
            if(robotCoords[identifier][i] < 0 || robotCoords[identifier][i] > 30)
                cout << "Sorry, the acceptable range of values is 1 to 30. Please enter the coordinate again.\n";
        } while (robotCoords[identifier][i] < 0 || robotCoords[identifier][i] > 30);
    }
    cout << endl;


    // Creating pipes based on the write labels mentioned in variables section
    for (int i = 0; i < totalRobots; i++) {
        if (i != identifier) {
            if (mkfifo(writeLabels[i], 0666) == -1) {
                if (errno != EEXIST) {
                    cout << "Error in creating FIFO pipe: " << writeLabels[i]; 
                    return 0;
                }
            }
        }
    }
        

    // Read/Write Seuqences for broadcasting coordinates
    // Iterate over all robots

    // W - R - R - R 
    // R - W - R - R
    // R - R - W - R
    // R - R - R - W


    for (int i = 0; i < totalRobots; i++) {
        
        // If it is the current robot's turn, then start doing the broadcast operation
        if (i == identifier) {

            cout << "Broadcasting my new coordinates (" << robotCoords[identifier][0] << "," << robotCoords[identifier][1] << ") ";
            cout << "to all robots on the following pipes: ";

            // Broadcast current robot's coordinates to all robots
            for (int j = 0; j < totalRobots; j++) {
                if (j != i) {
                    cout << writeLabels[j] << " ";
                    fileDescriptor = open(writeLabels[j], O_WRONLY);
                    if (write(fileDescriptor, &robotCoords[i], sizeof(int) * 2) == -1) {
                        cout << "Error in writing coordinate into FIFO pipe: " << writeLabels[i];
                    }
                }
            }

            cout << endl;

        // Else, read coordinates sent from other robots and compute distance.
        } else {
            fileDescriptor = open(readLabels[i], O_RDONLY);
            if (read(fileDescriptor, &robotCoords[i], sizeof(int) * 2) == -1) {
                cout << "Error in reading coordinate from FIFO pipe: " << readLabels[i];
            }
            
            int x = robotCoords[i][0] - robotCoords[identifier][0]; 
            int y = robotCoords[i][1] - robotCoords[identifier][1]; 
            euclideans[i] = sqrt(pow(x, 2) + pow(y, 2));

            cout << "Coordinates (" << robotCoords[i][0] << "," << robotCoords[i][1] << ") received from Pipe: " << readLabels[i] << ". ";
            cout << "Distance = " << euclideans[i] << " units. " << endl;
        }
    }


    cout << "\nSending messages to robots...";
    // Iterate over all robots
    for (int i = 0; i < totalRobots; i++) {
        
        // If it is current robot's turn, then start doing the broadcast operation
        if (i == identifier) {

            // Broadcast current robot's ID to all robots
            for (int j = 0; j < totalRobots; j++) {
                if (j != i) {

                    // Send 0 to all robots which are more than 10 units aray from the robot
                    if (euclideans[j] > 10) {
                        fileDescriptor = open(writeLabels[j], O_WRONLY);
                        if (write(fileDescriptor, &dontRead, sizeof(int)) == -1) {
                            cout << "Error in writing ID into FIFO pipe: " << writeLabels[i];
                        }

                    // Send robot's own id to all robots which are less than 10 units aray from the current robot
                    } else {
                        fileDescriptor = open(writeLabels[j], O_WRONLY);
                        if (write(fileDescriptor, &RobotID[i], sizeof(int)) == -1) {
                            cout << "Error in writing ID into FIFO pipe: " << writeLabels[i];
                        }

                    }
                }
            }

        // Else, read id's sent from other robots
        } else {
            fileDescriptor = open(readLabels[i], O_RDONLY);
            if (read(fileDescriptor, &RobotID[i], sizeof(int)) == -1) {
                cout << "Error in reading ID from FIFO pipe: " << readLabels[i];
            }
        }
    }


    cout << "...done!\n\n";

    // This is done to synchronize the robots near the end
    usleep(1000); 
    
    // Display messages received 
    for(int i = 0; i < totalRobots; i++) {
        if (identifier != i && RobotID[i] != 0) {
            cout << "Message Received: " << "Hello " << RobotID[i] << ", we are neighbours!" << endl; 
        }
    }


    return 0;
}