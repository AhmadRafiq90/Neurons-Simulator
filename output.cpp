#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <cmath>
#include "general.cpp"
using namespace std;
double *Activations;
int threadsDone = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *Neuronfunc(void *);

double *getInputVals(double);

int main(int argc, char *argv[])
{
    unlink("outPipe");

    if (argc == 1)
        return 0;

    cout << "Hello from output layer" << endl;
    const int BufSize = 400, numNeurons = 1;
    char *buf = new char[BufSize];
    const char *numTotallayers = argv[0], *CurrLayer = argv[1], *pipe_name = argv[2];
    int pipe_fd = read_pipe(pipe_name);
    sem_t *writing = sem_open(lock, 0);

    sem_wait(writing);
    if (read(pipe_fd, buf, BufSize) < 0)
    {
        cout << "Reading failed" << endl;
        exit(EXIT_FAILURE);
    }

    close(pipe_fd);
    cleanPipes();
    cout << "Output pipe data " << buf << endl;
    const int prev_count = countValues(buf, BufSize) + 1;
    double **weights;
    allocate2D(weights, prev_count, numNeurons);
    ifstream ifile("Output_Weights.txt");
    for (int i = 0; i < prev_count; i++)
        weights[i] = ReadNeurons(ifile, numNeurons);
    double *prev_Layer = new double[prev_count];

    for (int i = 0, index = 0; i < prev_count; i++)
        prev_Layer[i] = splitstring(buf, index);

    if (string(argv[3]) == "break")
    {
        Activations = new double[numNeurons];
        pthread_t *neurons = new pthread_t[numNeurons];

        for (int i = 0; i < numNeurons; i++)
        {
            int *val = new int(i);
            Thread_Data *data = new Thread_Data;
            data->i = *val;
            data->prevLayer = prev_Layer;
            data->weights = weights;
            data->size = prev_count;
            pthread_create(&neurons[i], nullptr, Neuronfunc, (void *)data);
        }

        while (threadsDone < numNeurons)
            ;
        cout << "Final output is " << Activations[0] << endl;
        exit(0);
    }
    else
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            // Giving back flag to hidden layer to backpropagate the values.
            const char *flag = "back";
            if (execl("hidden", numTotallayers, CurrLayer, flag, nullptr) == -1)
            {
                perror("execl");
                exit(EXIT_FAILURE);
            }
        }
        else if (pid > 0)
        {
            Activations = new double[numNeurons];
            pthread_t *neurons = new pthread_t[numNeurons];

            for (int i = 0; i < numNeurons; i++)
            {
                int *val = new int(i);
                Thread_Data *data = new Thread_Data;
                data->i = *val;
                data->prevLayer = prev_Layer;
                data->weights = weights;
                data->size = prev_count;
                pthread_create(&neurons[i], nullptr, Neuronfunc, (void *)data);
            }

            while (threadsDone < numNeurons)
                ;

            double *newInps = getInputVals(Activations[0]);

            pipe_fd = make_write_pipe("outPipe");
            if (!writeToPipe(pipe_fd, newInps, 2))
            {
                cout << "Writing failed" << endl;
                exit(EXIT_FAILURE);
            }
            sem_post(writing);
            pthread_exit(0);
        }
        else
            cout << "Could not fork" << endl;
    }
}

double *getInputVals(double value)
{
    double *ptr = new double[2];
    ptr[0] = (pow(value, 2) + value + 1) / 2;
    ptr[1] = (pow(value, 2) - value) / 2;
    return ptr;
}

void *Neuronfunc(void *arg)
{
    Thread_Data *data = (Thread_Data *)arg;
    double val = 0.0;
    pthread_mutex_lock(&mutex);
    for (int j = 0; j < data->size; j++)
    {
        cout << data->prevLayer[j] << ' ' << data->weights[j][data->i] << ' ' << data->i << ' ';
        val += (data->prevLayer[j] * data->weights[j][data->i]);
        if (abs(val) < 1e-9)
        {
            val = 0.0; // If the result is within the tolerance, set it to zero
        }
        // cout << val << ' ';
        cout.flush();
    }
    Activations[data->i] = val;
    cout << endl
         << "output is "
         << val << endl;
    threadsDone++;
    pthread_mutex_unlock(&mutex);
    pthread_exit(0);
}
