#include <mpi.h>
#include <iostream>
#include <climits>
#include <algorithm>
#include <cstddef>
#include <exception>

#define SIZE 10  

class custom_exception : public std::exception
{
  virtual const char* what() const throw()
  {
    return "Method is not implemented 'couse i'm feeling too lazy";
  }
} too_lazy_exception;

enum Operations
{
    FIND_MAX = 1,
    FIND_MIN = 2,
    FIND_AVG = 3,
    PRINT = 5,
    NULL_OPERATION = INT_MAX
};

struct Message
{
    int signal;
    int array_length;
    int array_of_ints[SIZE];
    Operations operation;
};

// constructor for Message struct
Message create_message(int _signal, int _array_length, int *_array_of_ints, Operations _operation)
{
    Message msg;
    msg.signal = _signal;
    msg.array_length = _array_length;
    memcpy(msg.array_of_ints, _array_of_ints, sizeof(int) * _array_length);
    msg.operation = _operation;
    return msg;
}

void sendCommand(Message _msg);
char *charificationCommand(int &_size_of_message, Message &_message);
void listener(int &_size_of_message, char *_charifiedObject, Message *_message);

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    Message *message = nullptr;
    char *charifiedObject = nullptr;
    int size_of_message;

    if (world_rank == 0) 
    {
        int *arr = (int *)malloc(sizeof(int) * SIZE);
        for (int i = 0; i < SIZE; i++)
            *(arr + i) = i;
        Message msg_do_max = create_message(0, SIZE, arr, FIND_MAX);
        Message msg_do_min = create_message(0, SIZE, arr, FIND_MIN);
        Message msg_abort = create_message(9, 0, nullptr, NULL_OPERATION);

        sendCommand(msg_do_max);
        sendCommand(msg_do_min);
        sendCommand(msg_abort);
    } 
    else if (world_rank == 1) 
    {
        listener(size_of_message, charifiedObject, message);
    }
    MPI_Finalize();
}

void sendCommand(Message _msg)
{
    int size_of_message = 0;
    char *charifiedObject = charificationCommand(size_of_message, _msg);

    // to send only one message we could make a rule that tells no matter what
    // the length of the message is e.g. 1kB
    MPI_Send(&size_of_message, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    MPI_Send(charifiedObject, size_of_message, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
}

char *charificationCommand(int &_size_of_message, Message &_message)
{
    _size_of_message = sizeof(_message);
    char *_charifiedObject = (char *)malloc(_size_of_message);
    memcpy(_charifiedObject, &_message, _size_of_message);
    return _charifiedObject;
}

void listener(int &_size_of_message, char *_charifiedObject, Message *_message)
{
    MPI_Recv(&_size_of_message, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    _charifiedObject = (char *)malloc(_size_of_message);

    MPI_Recv(_charifiedObject, _size_of_message, MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    _message = (Message *)malloc(_size_of_message);
    memcpy(_message, _charifiedObject, _size_of_message);
    
    free(_charifiedObject);
    _charifiedObject = nullptr;

    if (_message->signal == 9)
    {
        std::cout << "Received sigkill. Exiting..." << std::endl;
        MPI_Finalize();
        exit(0);
    }

    switch(_message->operation)
    {
        case FIND_MAX: 
            std::cout << "Largest number in the array is " 
                << *(std::max_element(_message->array_of_ints, 
                _message->array_of_ints + _message->array_length)) << std::endl;
            break;
        case FIND_MIN:
            std::cout << "Smallest number in the array is " 
                << *(std::min_element(_message->array_of_ints, 
                _message->array_of_ints + _message->array_length)) << std::endl;
            break;
        case FIND_AVG:
            throw too_lazy_exception;
            break;
        default:
            std::cout << "Unsupported operation selected" << std::endl; 
            break;
    }

    free(_message);
    _message = nullptr;

    // a dummy RAM-eating recursion
    listener(_size_of_message, _charifiedObject, _message);
}