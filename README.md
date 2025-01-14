## Neural Network with Parallel Processing

This project implements a basic neural network using process-based parallelism. Each layer's forward pass is handled by forking new processes, while neurons are represented using pthreads for concurrency. The backward pass is also parallelized using fork, enabling efficient process management and computation in a multi-threaded environment.

## Files in the Project

    general.cpp: Contains the general framework and utilities for the neural network.
    hidden.cpp: Implements the hidden layer computations and activations.
    input.txt: Input data for the neural network.
    main.cpp: Main entry point of the program, handling network training and execution.
    output.cpp: Output processing of the neural network results.
    Output_Weights.txt: File containing the inital output layer weights.
    Weights1.txt to Weights5.txt: Weight files for different layers and configurations.
    temp.txt: A smaller weights file to initially run the program on.

## Dependencies

This project is developed and tested on Ubuntu. The following dependencies are required to build and run the project:

    g++ (GNU Compiler Collection)
    pthread (for multithreading)

## Install Dependencies

Run the following commands to install the necessary dependencies:

```
sudo apt update
sudo apt install g++ make
sudo apt install libpthread-stubs0-dev
```

## Compilation

    Open a terminal and navigate to the project directory.

    Compile the project using g++:

```g++ -o neural_network main.cpp general.cpp hidden.cpp output.cpp -lpthread```

This will create an executable file named neural_network in the current directory.

## Running the Project

To run the project, use the following command in the terminal:

./neural_network

## Expected Output

After running the program, the weights and losses will be displayed on the console

## How the Program Works

    Input Handling: The neural network reads the input data from temp.txt.
    Forward Pass: Each layer's forward pass is computed in a separate process, utilizing fork-based parallelism for efficient processing.
    Neurons & Threads: Each neuron is represented using a pthread to parallelize the operations for each neuron in a layer.
    Backward Pass: The backward pass is parallelized using fork as well, ensuring efficient backpropagation across the network.
    Output: The program displays the weights and losses on the console.

## Project Structure

The project directory should look like this:

```
/neural-network-project
├── general.cpp
├── hidden.cpp
├── input.txt
├── main.cpp
├── output.cpp
├── Output_Weights.txt
├── temp.txt
├── Weights1.txt
├── Weights2.txt
├── Weights3.txt
├── Weights4.txt
└── Weights5.txt
```


## Troubleshooting

    Missing libpthread: If you encounter issues related to pthread, ensure that the required libraries are installed via the package manager.

    Incorrect Input Format: Ensure that the input.txt or temp.txt file is properly formatted and contains the correct data as per the neural network's requirements.
