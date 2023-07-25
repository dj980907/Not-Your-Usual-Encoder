
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <ctype.h>
#include <math.h>

typedef struct Task{
    char* taskPointer;
    int id;
    int length;
}Task;

//global variables
//this keeps track of how many tasks there are
int taskAmount = 0;

//this is completed task array
char** completedTasks;
int completedindex = 0;

//this is the length of each task queue
int length[250000];
int lengthIndex = 0;

Task TaskQueue[250000];
int TaskQueueIndex = 0;

//-----------------------------------------------------
// //mutex and condition variable

//this is mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//condition variable indicating that the buffer is not empty
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

pthread_cond_t ready = PTHREAD_COND_INITIALIZER;

//keeps track of how many elements are in the buffer
int Buffercount = 0;


//this is the new attempt
void* encrypt(){

    while(true){

        // lock the mutex
        pthread_mutex_lock(&mutex);

        //wait for the producer to send signal
        while(Buffercount == 0){
            pthread_cond_wait(&not_empty, &mutex);
        }

        // //this is the array that stores the final values within this function
        char *resultArray = malloc(4096 * sizeof(char));

        //index starts at 0
        int arrayIndex = 0;

        //setting the global array index to be the task queue index 
        int globalArrayIndex = TaskQueueIndex;

        // printf("executing taskque[%d]\n",globalArrayIndex);

        //increment the index of task queue so that other consumers can work 
        TaskQueueIndex++;

        //this will be the length of the completed task
        int localLengthIndex = lengthIndex;
        lengthIndex++;

        //decrement the buffer count so that it keeps track of
        //how many elements are in the buffer
        Buffercount--;

        //declare variables that I will use
        unsigned char count = 0;
        char curr_char;

        // Initialize current character and count
        curr_char = TaskQueue[globalArrayIndex].taskPointer[0];
        count = 0;

        for(int i = 0; i < TaskQueue[globalArrayIndex].length; i++){

            if(TaskQueue[globalArrayIndex].taskPointer[i] == curr_char){
                count++;
            }
            else{
                //add it to the char array
                resultArray[arrayIndex] = curr_char;
                //increment the index
                arrayIndex++;
                //add it to the array
                resultArray[arrayIndex] = count;
                arrayIndex++;
                // Set the current character and count to the new values
                curr_char = TaskQueue[globalArrayIndex].taskPointer[i];
                count = 1;
            }


        }
        resultArray[arrayIndex] = curr_char;
        //increment the index
        arrayIndex++;
        //add it to the array
        resultArray[arrayIndex] = count;
        arrayIndex++;

        //put it in the completed task array
        length[localLengthIndex] = arrayIndex;
        completedTasks[TaskQueue[globalArrayIndex].id] = resultArray;

        pthread_cond_signal(&ready);

        pthread_mutex_unlock(&mutex);
    }

}


int main(int argc, char* argv[]){

    // taskQueue = malloc(250000 * sizeof(char*));
    completedTasks = malloc(250000 * sizeof(char*));

    if(strcmp(argv[1], "-j") == 0){

        //get the integer value of the argument right after
        int value = atoi(argv[2]);

        //create thread pool
        pthread_t th[value];

        //create consumer thread
        for(int i = 0; i < value; i++){
            pthread_create(&th[i], NULL, &encrypt, NULL);
        }

        int j;

        //go through arguments
        for(j = 3; j < argc; j++){

            // Open file
            int fd = open(argv[j], O_RDONLY);
            if (fd == -1){
                printf("open file error\n");
                return 1;
            }

            // Get file size
            struct stat sb;
            if (fstat(fd, &sb) == -1){
                printf("size error\n");
                return 1;
            }

            int chunkSize;
            int arraySlots;
            int offset = 0;
            if(sb.st_size < 4096){
                chunkSize = sb.st_size;
                arraySlots = 1;
            }
            else{
                //calculate the chunk size
                chunkSize = 4096;
                double before = sb.st_size / (double)chunkSize;
                arraySlots = (int)ceil(sb.st_size / (double)chunkSize); 
            }
            int temp = sb.st_size;

            for(int i = 0; i < arraySlots; i++){


                if(temp > 4096){
                    chunkSize = 4096;
                    temp = temp - 4096;
                }
                else{
                    chunkSize = temp;
                }

                // printf("hi\n");

                //lock the mutex
                pthread_mutex_lock(&mutex);

                //insert task into task queue
                Task t;
                t.taskPointer = mmap(NULL, chunkSize, PROT_READ, MAP_PRIVATE, fd, offset);
                t.id = taskAmount;
                t.length = chunkSize;
                TaskQueue[taskAmount] = t;
                
                //increment the buffer count
                Buffercount++;

                //increment the regular index so that the next data goes in the right spot
                taskAmount++;

                //increment the offset
                offset += 4096;

                //send the condition
                pthread_cond_signal(&not_empty);

                //unlock the mutex
                pthread_mutex_unlock(&mutex);
            }

            //close the file 
            close(fd);


        }

        char lastChar;
        unsigned char lastCount;
        char lastArray[2];
        bool first = true;
        int q = 0;

        while(q != taskAmount){
            pthread_mutex_lock(&mutex);
            while(completedTasks[q] == NULL){
                pthread_cond_wait(&ready, &mutex);
            }

            if(first){
                //write everything upto the last 2 
                //this could be the problem!!!!!!!!
                fwrite(completedTasks[q], sizeof(char), length[q] - 2, stdout);
                //this will be the last character
                lastChar = completedTasks[q][length[q]-2];

                //this is the occurrence of that character
                lastCount = completedTasks[q][length[q]-1];

                //store it in the array called last array
                lastArray[0] = lastChar;
                lastArray[1] = lastCount;

                //set the first flag to be true so that this code doesn't run again
                first = false;
            }
            else{
                // if first chunk's last character is equal to 
                //second chunk's first character
                if(lastChar == completedTasks[q][0]){

                    // second chunk's character count += first chunk's character count
                    //i think the issue is here but i am not sure why
                    completedTasks[q][1] += lastCount;

                    //write everything else except the last 2 members
                    fwrite(completedTasks[q], sizeof(char), length[q] - 2, stdout);

                    //this is the last character
                    lastChar = completedTasks[q][length[q]-2];
                    //this is the last character's occurrence
                    lastCount = completedTasks[q][length[q]-1];

                    //store it in the lastArray
                    lastArray[0] = lastChar;
                    lastArray[1] = lastCount;
                }
                else{
                    //if the first of the second result is not the same 
                    //as the last of the previous result
                    //write the last array
                    fwrite(lastArray, sizeof(char), 2, stdout);

                    //write everything up to the last 2
                    fwrite(completedTasks[q], sizeof(char), length[q] - 2, stdout);
                    lastChar = completedTasks[q][length[q]-2];
                    lastCount = completedTasks[q][length[q]-1];
                    lastArray[0] = lastChar;
                    lastArray[1] = lastCount;
                }
            }
            q++;
            pthread_mutex_unlock(&mutex);
        }

        fwrite(lastArray, sizeof(char), 2, stdout);
    }
    
    //this is exactly the same as the previous one that got 60 on milestone 2

    //until 128 is ok but 129 is not ok
    else{
        //this is the array that stores the final values
        char *resultArray = malloc(4096 * sizeof(char));
        //index starts at 0
        int arrayIndex = 0;

        //declare variables that I will use
        unsigned char count;
        char curr_char;
        char prev_char;
        bool first = true;

        for(int j = 1; j < argc; j++){
            
            // Open file
            int fd = open(argv[j], O_RDONLY);
            if (fd == -1){
                printf("Something went wrong opeing the file homie\n");
                return 1;
            }

            // Get file size
            struct stat sb;
            if (fstat(fd, &sb) == -1){
                printf("Something went wrong opeing the file homie\n");
                return 1;
            }

            // Map file into memory
            char *addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
            if (addr == MAP_FAILED){
                printf("Something went wrong opeing the file homie\n");
                return 1;
            }

            //close the file 
            close(fd);

            // Initialize current character and count
            curr_char = addr[0];
            count = 0;

            if(curr_char != prev_char){
                if(!first){
                    fwrite(resultArray, sizeof(char), 2, stdout);
                    arrayIndex = 0;
                }
            }

            for(int i = 0; i < sb.st_size; i++){

                if(addr[i] == curr_char){
                    count++;
                }
                else{

                    if(prev_char == curr_char){
                        resultArray[1] = resultArray[1] + count;
                        fwrite(resultArray, sizeof(char), 2, stdout);
                        arrayIndex = 0;
                        curr_char = addr[i];
                        count = 1;
                    }
                    else{

                        //add it to the char array
                        resultArray[arrayIndex] = curr_char;
                        //increment the index
                        arrayIndex++;
                        //add it to the array
                        resultArray[arrayIndex] = count;
                        arrayIndex++;
                        fwrite(resultArray, sizeof(char), 2, stdout);
                        arrayIndex = 0;
                        // Set the current character and count to the new values
                        curr_char = addr[i];
                        count = 1;
                    }
                }
            }

            if(curr_char == prev_char){
                resultArray[1] = resultArray[1] + count;
            }
            
            //add to array
            resultArray[arrayIndex] = curr_char;
            //increment index
            arrayIndex++;
            //add to the array
            resultArray[arrayIndex] = count;
            arrayIndex++;
            //set the prev char
            prev_char = curr_char;
            first = false;
            // fwrite(resultArray, sizeof(char), 2, stdout);
            // arrayIndex = 0;
        }
        fwrite(resultArray, sizeof(char), 2, stdout);
        // free(resultArray);
    }
    free(completedTasks);
}