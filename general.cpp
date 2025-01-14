#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fstream>
#include <sys/resource.h>
#include <sys/time.h>
using namespace std;
const char *lock = "/my_mutex";

struct Thread_Data
{
    double **weights;  // weights of all the neurons.
    double *prevLayer; // activation functions of the previous layers.
    int i;             // weight of the current neuron.
    int size;          // size of previous layer.
};

int make_write_pipe(const char *name)
{
    if (mkfifo(name, 0666) < 0)
    {
        cout << "Name of pipe is " << name << endl;
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }

    return open(name, O_WRONLY);
}

int read_pipe(const char *name)
{
    int ret = open(name, O_RDONLY);
    if (ret <= 0)
    {
        cout << "Pipe does not exist" << endl;
        exit(EXIT_FAILURE);
    }
    return ret;
}

// Takes a comma seperated string
// and then counts all values in that string.

int countValues(char const *data, int length)
{
    int count = 0;
    for (int i = 0; i < length; i++)
        if (data[i] == ',')
            count++;
    return count;
}
// Creates a named semaphore in the file system.
// Initital value would be the same as the parameter value.
sem_t *createNamedSemaphore(unsigned int value)
{
    sem_t *sem = sem_open(lock, O_CREAT, 0644, value);
    if (sem == SEM_FAILED)
    {
        perror("sem_open");
        return nullptr;
    }
    else
        return sem;
}

// Takes a comma seperated string and stores
// all values seperately in a new array.

template <class T>
double splitstring(T str, int &index)
{
    string temp;
    while (str[index] != ',' && str[index] != '\0')
        temp += str[index++];
    index++;
    return stod(temp);
}

// Reads neuron weights from file and stores them in
// a dynamic 2d array.
double *ReadNeurons(ifstream &obj, const int neurons)
{
    double *data = new double[neurons]{};
    if (obj)
    {
        string dat;
        getline(obj, dat);
        int ind = 0, index = 0;
        while (index < dat.length())
            data[ind++] = splitstring(dat, index);
        return data;
    }
    else
    {
        cout << "Could not open file" << endl;
        exit(EXIT_FAILURE);
    }
}
// Takes a pipe fd as parameter, buffer/array to be written and its size
// and then performs writing operation and terminates with null character.
template <class T>
bool writeToPipe(int fd, T *buffer, int size)
{
    for (int i = 0; i < size; i++)
    {
        string w;
        if (i != size - 1)
            w = to_string(buffer[i]) + ",";
        else
            w = to_string(buffer[i]);
        cout << "Main writing " << w << endl;
        if (write(fd, w.c_str(), w.length()) <= 0)
            return false;
    }
    write(fd, "", 1);
    close(fd);
    return true;
}

// This function returns the status of a process whether it is
// running or not without blocking the calling function.
bool isChildRunning(pid_t pid)
{
    return !waitpid(pid, nullptr, WNOHANG);
}

// Checks whether a pipe exists or not.
bool pipeExists(const char *path)
{
    if (access(path, F_OK) != -1)
        return true;
    else
        return false;
}

// Removes already used pipes from system if they exist.
void cleanPipes()
{
    string t;
    for (int i = 1; i <= 10; i++)
    {
        t = "pipe" + to_string(i);
        unlink(t.c_str());
    }
}

// Allocates 2D array of given row and col size.
template <class T>
void allocate2D(T **&arr, int row, int col)
{
    arr = new T *[row];
    for (int i = 0; i < row; i++)
        arr[i] = new T[col]{};
}