#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "general.cpp"
using namespace std;
string comms = "pipe";

int main(int argc, char *argv[])
{
    // unlink((comms + to_string(1)).c_str());
    cleanPipes();
    sem_unlink(lock);
    const int num_neurons = 2;
    const char *totalhiddenLayers = "6";
    double *weights;
    sem_t *writing = createNamedSemaphore(1);

    // Handling case when one complete back and forward pass has been completed
    // so infinite fork and exec does not occur.

    if (argc > 1)
    {
        double w1 = strtod(argv[0], nullptr), w2 = strtod(argv[1], nullptr);
        weights = new double[2]{w1, w2};
        sem_wait(writing);
        pid_t pid = fork();
        if (pid == 0)
        {
            if (execl("hidden", totalhiddenLayers, "1", "forward", "break", nullptr) == -1)
            {
                perror("execl");
                exit(EXIT_FAILURE);
            }
        }
        else if (pid > 0)
        {
            int pipe_fd = make_write_pipe((comms + to_string(1)).c_str());
            if (pipe_fd > 0)
            {
                if (!writeToPipe(pipe_fd, weights, num_neurons))
                {
                    cout << "Writing failed" << endl;
                    exit(EXIT_FAILURE);
                }
                write(pipe_fd, "", 1);
                close(pipe_fd);
                sem_post(writing);
            }
            while (isChildRunning(pid))
                pthread_yield();
        }
        else
            cout << "Fork error" << endl;
    }
    else
    {
        ifstream obj("temp.txt");
        weights = ReadNeurons(obj, num_neurons);
        sem_wait(writing);
        pid_t pid = fork();
        if (pid == 0)
        {
            if (execl("hidden", totalhiddenLayers, "1", "forward", "continue", nullptr) == -1)
            {
                perror("execl");
                exit(EXIT_FAILURE);
            }
        }
        else if (pid > 0)
        {
            int pipe_fd = make_write_pipe((comms + to_string(1)).c_str());
            if (pipe_fd > 0)
            {
                if (!writeToPipe(pipe_fd, weights, num_neurons))
                {
                    cout << "Writing failed" << endl;
                    exit(EXIT_FAILURE);
                }
                write(pipe_fd, "", 1);
                close(pipe_fd);
                sem_post(writing);
            }
            while (isChildRunning(pid))
                pthread_yield();
        }
        else
            cout << "Fork error" << endl;
    }
}