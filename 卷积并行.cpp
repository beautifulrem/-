#include <iostream>
#include <fstream>
#include <pthread.h>
#include <chrono>

using namespace std;

#define MAX_THREADS 16

//定义互斥锁
pthread_mutex_t mutex_lock;

// 定义一个结构体，用于将参数传递给线程
struct ThreadArg
{
    int id;
    int rows;
    int cols;
    int** input;
    int** output;
    int** kernel;
    int kernelSize;
    int iterations;
};

//定义卷积操作
void convolve(int** input, int** output, int** kernel, int rows, int cols, int kernelSize)
{
    // 计算卷积核的中心坐标
    int kernelOffset = kernelSize / 2;

    // 遍历输入矩阵
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            int sum = 0;

            // 遍历卷积核
            for (int k = 0; k < kernelSize; k++)
            {
                for (int l = 0; l < kernelSize; l++)
                {
                    // 计算卷积核相对于当前像素的偏移
                    int inputRow = i + k - kernelOffset;
                    int inputCol = j + l - kernelOffset;

                    // 边界检查
                    if (inputRow >= 0 && inputRow < rows && inputCol >= 0 && inputCol < cols)
                    {
                        // 计算卷积和
                        sum += input[inputRow][inputCol] * kernel[k][l];
                    }
                }
            }

            // 将卷积和存储在输出矩阵中
            output[i][j] = sum;
        }
    }
}

// 定义线程函数，执行卷积操作
void* threadFunction(void* arg)
{
    ThreadArg* threadArg = (ThreadArg*)arg;

    // 计算每个线程负责的像素行数
    int startRow = (threadArg->id * threadArg->rows) / MAX_THREADS;
    int endRow = ((threadArg->id + 1) * threadArg->rows) / MAX_THREADS;

    // 执行卷积操作
    for (int i = 0; i < threadArg->iterations; i++)
    {
        // 加锁
        pthread_mutex_lock(&mutex_lock);

        convolve(threadArg->input, threadArg->output, threadArg->kernel, threadArg->rows, threadArg->cols, threadArg->kernelSize);

        // 解锁
        pthread_mutex_unlock(&mutex_lock);
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
    // 获取命令行参数
    if (argc != 4)
    {
        cout << "Usage: " << argv[0] << " <input_file> <kernel_file> <iterations>" << endl;
        return 1;
    }

    // 打开输入文件
    ifstream inputFile(argv[1]);
    if (!inputFile)
    {
        cerr << "Error: Could not open input file." << endl;
        return 1;
    }

    // 读取输入矩阵
    int rows, cols;
    inputFile >> rows >> cols;

    int** input = new int* [rows];
    for (int i = 0; i < rows; i++)
    {
        input[i] = new int[cols];
        for (int j = 0; j < cols; j++)
        {
            inputFile >> input[i][j];
        }
    }

    inputFile.close();

    // 打开卷积核文件
    ifstream kernelFile(argv[2]);
    if (!kernelFile)
    {
        cerr << "Error: Could not open kernel file." << endl;
        return 1;
    }

    // 读取卷积核
    int kernelSize;
    kernelFile >> kernelSize;

    int** kernel = new int* [kernelSize];
    for (int i = 0; i < kernelSize; i++)
    {
        kernel[i] = new int[kernelSize];
        for (int j = 0; j < kernelSize; j++)
        {
            kernelFile >> kernel[i][j];
        }
    }

    kernelFile.close();

    // 创建输出矩阵
    int** output = new int* [rows];
    for (int i = 0; i < rows; i++)
    {
        output[i] = new int[cols];
    }

    // 初始化互斥锁
    pthread_mutex_init(&mutex_lock, NULL);

    // 创建线程
    int numThreads = min(MAX_THREADS, rows);
    pthread_t threads[numThreads];
    ThreadArg threadArgs[numThreads];

    int rowsPerThread = rows / numThreads;

    for (int i = 0; i < numThreads; i++)
    {
        threadArgs[i].id = i;
        threadArgs[i].rows = rows;
        threadArgs[i].cols = cols;
        threadArgs[i].input = input;
        threadArgs[i].output = output;
        threadArgs[i].kernel = kernel;
        threadArgs[i].kernelSize = kernelSize;
        threadArgs[i].iterations = stoi(argv[3]);

        // 创建线程
        int result = pthread_create(&threads[i], NULL, threadFunction, &threadArgs[i]);
        if (result != 0)
        {
            cerr << "Error: Could not create thread." << endl;
            return 1;
        }
    }

    // 等待线程完成
    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // 销毁互斥锁
    pthread_mutex_destroy(&mutex_lock);

    // 输出结果矩阵
    cout << "Result:" << endl;
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            cout << output[i][j] << " ";
        }
        cout << endl;
    }

    // 释放内存
    for (int i = 0; i < rows; i++)
    {
        delete[] input[i];
        delete[] output[i];
    }
    delete[] input;
    delete[] output;

    for (int i = 0; i < kernelSize; i++)
    {
        delete[] kernel[i];
    }
    delete[] kernel;

    return 0;
}
