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
#include <fstream>


using namespace std;

int main() {

    // Total Robots
    int totalRobots = 4;

    // Roll number string variables
    char rollnumber[100];
    char receivedNumber[100];

    // Roll number integer storages
    int numbersOnly[totalRobots];
    int numbersOnlyChild[totalRobots];

    // Pipes for data transfer between the 2 processes
    int childPipe[2];
    int parentPipe[2];

    // Pipe creation
    if (pipe(childPipe) == -1)
        cout << "Error in creating child pipe.\n"; 
    if (pipe(parentPipe) == -1)
        cout << "Error in creating parent pipe.\n"; 

    pid_t pid = fork();

    // Child Process
    if (pid == 0) {
        
        // String input
        for (int i = 0; i < totalRobots; i++) {
            cout << "Please enter your roll number: ";
            cin >> rollnumber;
            rollnumber[strlen(rollnumber) + 1] = '\0';

            // Send to parent
            if (write(childPipe[1], rollnumber, sizeof(char) * 8) == -1)
                cout << "Error in writing roll number into child pipe.\n";
            
        }

        // Display robot ids which are received from parent
        if (read(parentPipe[0], numbersOnlyChild, sizeof(int) * 4) == -1)
            cout << "Error in reading roll number from parent pipe.\n";
        
        for (int i = 0; i < totalRobots; i++)
            cout << "robot_id_" << numbersOnlyChild[i] << " ";
        cout << endl;

        // Write robot id's to file
        ofstream myFile("rollnumbers.txt", ios::trunc);
        if (myFile.is_open()) {
            for(int i = 0; i < totalRobots; i++){
                myFile << numbersOnlyChild[i] << " " ;
            }
            myFile.close();
        } else {
            cout << "Error in opening file";
            return 0;
        }
        
        exit(0);

    } else if (pid > 0) {

        // Extract roll number from string which you get from child process
        for (int i = 0; i < totalRobots; i++) {
            
            if (read(childPipe[0], receivedNumber, sizeof(char) * 8) == -1)
                cout << "Error in receiving roll number from child pipe.\n";
        

            // Extract the number from it and store it in an integer array
            string temp = receivedNumber;
            string temp2 = temp.substr(3, 4);
            numbersOnly[i] = stoi(temp2);
        }

        // Send back to child
        if (write(parentPipe[1], numbersOnly, sizeof(int) * 4) == -1)
            cout << "Error in writing roll number into parent pipe.\n";

        wait(NULL);
    } else {
        cout << "Error in forking." << endl;
        return 0;
    }
    
    return 0;
}