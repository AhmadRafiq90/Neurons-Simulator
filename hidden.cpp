#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include "general.cpp"
using namespace std;
double *Activations;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int threadsDone = 0;
const int numNeurons = 8;
void WritingFunc(int, double *, double **, int, sem_t *);

void *Neuronfunc(void *);

// argv[0] no of total hidden layers to execute.
// argv[1] no of current layer executing.
// argv[2] Mode -> forward or backpropagation.
// argv[3] Break variable to break in second pass to avoid infinite fork.

int main(int argc, char *argv[])
{
    const int length = 200;
    const int totalLayers = atoi(argv[0]);
    int layer_no = atoi(argv[1]);
    string direction = string(argv[2]), s;

    if (direction == "back" && layer_no > totalLayers)
    {
        cout << "Back flag received" << endl;
        s = "outPipe";
    }
    else
        s = "pipe" + to_string(layer_no);

    sem_t *writing = sem_open(lock, 0);

    cout << endl
         << "Hello from layer " << layer_no << endl;

    // Busy wait to check if pipe exists or not.
    while (!pipeExists(s.c_str()))
        pthread_yield();

    if (layer_no == 1 && direction == "forward")
    {
        int pipe_fd = read_pipe(s.c_str());

        if (pipe_fd > 0)
        {
            char *buf = new char[length];
            int bytes_read = 0, index = 0, ind = 0;

            sem_wait(writing);
            read(pipe_fd, buf, length);
            cout << "layer no " << layer_no << " read " << buf << endl;
            close(pipe_fd);
            const int char_count = countValues(buf, length) + 1;
            ifstream obj("Input.txt");
            double **weights;

            allocate2D(weights, char_count, numNeurons);
            for (int i = 0; i < char_count; i++)
                weights[i] = ReadNeurons(obj, numNeurons);

            double *prev_layer = new double[char_count]{};
            while (index < char_count)
                prev_layer[index++] = splitstring(buf, ind);

            pid_t pid = fork();

            if (pid == 0)
            {
                char buf[3]{};
                const char *flag = "forward";
                sprintf(buf, "%d", layer_no + 1);
                if (execl("hidden", argv[0], buf, flag, argv[3], nullptr) == -1)
                {
                    perror("execl");
                    exit(EXIT_FAILURE);
                }
            }

            else if (pid > 0)
            {
                WritingFunc(layer_no, prev_layer, weights, char_count, writing);
                while (isChildRunning(pid))
                    pthread_yield();
                pthread_exit(0);
            }

            else
                cout << "Fork error" << endl;
        }
        else
            cout << "Could not open pipe" << endl;
    }

    else if (layer_no <= totalLayers && direction == "forward")
    {
        int pipe_fd = read_pipe(s.c_str());

        if (pipe_fd > 0)
        {
            char *buf = new char[length];
            int bytes_read = 0, index = 0, ind = 0;

            sem_wait(writing);
            read(pipe_fd, buf, length);
            cout << "layer no " << layer_no << " read " << buf << endl;

            close(pipe_fd);
            const int char_count = countValues(buf, length) + 1;
            cout << "char_count is " << char_count << endl;
            ifstream obj("Weights" + to_string(layer_no - 1) + ".txt");
            double **weights;

            allocate2D(weights, char_count, numNeurons);
            for (int i = 0; i < char_count; i++)
                weights[i] = ReadNeurons(obj, numNeurons);

            double *prev_layer = new double[char_count]{};
            while (index < char_count)
                prev_layer[index++] = splitstring(buf, ind);

            pid_t pid = fork();

            if (pid == 0)
            {
                char buf[3]{};
                const char *flag = "forward";
                sprintf(buf, "%d", layer_no + 1);
                if (execl("hidden", argv[0], buf, flag, argv[3], nullptr) == -1)
                {
                    perror("execl");
                    exit(EXIT_FAILURE);
                }
            }

            else if (pid > 0)
            {
                WritingFunc(layer_no, prev_layer, weights, char_count, writing);
                while (isChildRunning(pid))
                    pthread_yield();
                pthread_exit(0);
            }

            else
                cout << "Fork error" << endl;
        }
        else
            cout << "Could not open pipe" << endl;
    }

    else if (layer_no > totalLayers && direction == "forward")
    {
        char buf[3]{};
        string pName = "pipe" + to_string(layer_no);
        sprintf(buf, "%d", layer_no);
        if (execl("output", argv[0], buf, pName.c_str(), argv[3], nullptr) == -1)
        {
            cout << "Exec failed" << endl;
            exit(EXIT_FAILURE);
        }
    }

    else if (layer_no > 1 && direction == "back")
    {
        int pipe_fd = read_pipe(s.c_str());
        char *buf = new char[length];
        int bytes_read = 0, index = 0, ind = 0;

        sem_wait(writing);
        read(pipe_fd, buf, length);
        cout << "layer no " << layer_no << " read " << buf << endl;
        close(pipe_fd);
        const int prev_count = countValues(buf, length) + 1;
        double *prev_layer = new double[prev_count];

        for (int i = 0, index = 0; i < prev_count; i++)
            prev_layer[i] = splitstring(buf, index);

        pid_t pid = fork();
        string Pname = "pipe" + to_string(layer_no - 1);

        if (pid == 0)
        {
            char buf[3]{};
            sprintf(buf, "%d", layer_no - 1);
            if (execl("hidden", argv[0], buf, "back", argv[3], nullptr) == -1)
            {
                perror("execl");
                exit(EXIT_FAILURE);
            }
        }

        else if (pid > 0)
        {
            int pipe_fd = make_write_pipe(Pname.c_str());
            if (!writeToPipe(pipe_fd, prev_layer, prev_count))
            {
                cout << "Writing failed" << endl;
                exit(EXIT_FAILURE);
            }
            sem_post(writing);
            while (isChildRunning(pid))
                pthread_yield();
        }

        else
            cout << "Could not fork" << endl;
    }
    else if (layer_no == 1 && direction == "back")
    {
        int pipe_fd = read_pipe(s.c_str());
        char *buf = new char[length];
        int bytes_read = 0, index = 0, ind = 0;

        sem_wait(writing);
        read(pipe_fd, buf, length);
        cout << "Layer no " << layer_no << " read from pipe " << buf << endl;

        close(pipe_fd);
        const int prev_count = countValues(buf, length) + 1;
        double *prev_layer = new double[prev_count];

        for (int i = 0, index = 0; i < prev_count; i++)
            prev_layer[i] = splitstring(buf, index);

        sem_post(writing);
        sem_close(writing);
        sem_unlink(lock);

        execl("input", to_string(prev_layer[0]).c_str(),
              to_string(prev_layer[1]).c_str(), nullptr);
    }
    return 0;
}

void WritingFunc(int layer_no, double *prev_layer, double **weights, int char_count, sem_t *writing)
{
    Activations = new double[numNeurons]{};
    pthread_t *neurons = new pthread_t[numNeurons];

    // This function will create seperate threads
    // for each neuron and then compute their
    // activation functions seperately and then
    // store them in the activations array.

    for (int i = 0; i < numNeurons; i++)
    {
        int *val = new int(i);
        Thread_Data *data = new Thread_Data;
        data->i = *val;
        data->prevLayer = prev_layer;
        data->weights = weights;
        data->size = char_count;
        pthread_create(&neurons[i], nullptr, Neuronfunc, (void *)data);
    }

    // Busy wait to allow every thread to complete.
    while (threadsDone < numNeurons)
        ;

    // Write activations of each neuron to pipes for next layers to work.

    string s = "pipe" + to_string(layer_no + 1);
    int pipe_fd = make_write_pipe(s.c_str());
    if (!writeToPipe(pipe_fd, Activations, numNeurons))
    {
        cout << "Writing failed" << endl;
        exit(EXIT_FAILURE);
    }

    sem_post(writing);
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
            val = 0.0; // If the result is within the tolerance, set it to zero

        cout.flush();
    }
    Activations[data->i] = val;
    cout << endl
         << "val is "
         << val << endl;
    threadsDone++;
    pthread_mutex_unlock(&mutex);
    pthread_exit(0);
}