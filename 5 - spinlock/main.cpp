#include <thread>
#include <barrier>
#include <vector>
#include <set>
#include <chrono>
#include <iomanip>
#include <condition_variable>

#include "list.hpp"

using namespace std;


void printCalculatedTime(const vector<vector<std::chrono::nanoseconds>>& calculated_time,
    const vector<size_t>& sizes,
    const vector<int32_t>& thread_num)
{
    cout << "Theads:  ";
    for (int i = 0; i < thread_num.size(); i++)
    {
        cout << thread_num[i] << ' ';
    }
    cout << '\n';

    for (int i = 0; i < sizes.size(); i++)
    {
        cout << "Size: " << std::setw(10) << std::left << sizes[i];
        for (int j = 0; j < thread_num.size(); j++)
        {
            cout << std::setw(15) << std::left << calculated_time[i][j] << ' ';
        }
        cout << '\n';
    }
    cout << '\n';
}

void Test_One()
{
    size_t n = 10000;
    vector<size_t> sizes = { 100, 1000, 10000, 100000, 1000000 };
    vector<int32_t> thread_num = { 1, 2, 5, 10, 20, 50, 100 };
    vector<vector<std::chrono::nanoseconds>> calculated_time(sizes.size(),
        vector<std::chrono::nanoseconds>(thread_num.size()));

    for (int s = 0; s < sizes.size(); s++)
    {
        size_t n = sizes[s];

        for (int k = 0; k < thread_num.size(); k++)
        {
            list<int32_t> li;
            vector<thread*> threads;
            int m = thread_num[k],
                part = n / m;
            barrier insert_barr(m);

            auto time_begin = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < m; i++)
            {
                threads.push_back(new thread([&](int i_) {
                    insert_barr.arrive_and_wait();
                    for (int j = part * i_; j < part * (i_ + 1); j++)
                    {
                        if (j % 2 == 0)
                            li.push_back(j);
                        else
                            li.push_front(j);
                    }
                    }, i));
            }

            for (int i = 0; i < m; ++i) {
                threads[i]->join();
            }

            assert(li.size() == n, "Incorrect size of list");
            //cout << "All inserted \n\n";

            //save time
            auto time_end = std::chrono::high_resolution_clock::now();
            auto time_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(time_end - time_begin);
            calculated_time[s][k] = time_elapsed;

            // check inserted values => they all must be different
            set<int32_t> check;
            for (auto it = li.begin(); it != li.end(); it++)
            {
                check.insert(*it);
            }

            assert(check.size() == n, "Duplicated values");

            for (auto it = li.begin(); it != li.end(); it++)
            {
                if (*it < 0 || *it > n)
                {
                    assert(false, "Incorrect values");
                }
            }
        }

        cout << n << "  OK\n";
    }

    printCalculatedTime(calculated_time, sizes, thread_num);
    cout << "Test One OK\n" << "______________________________\n\n";
}

void Test_Two()
{
    size_t n = 10000;
    vector<size_t> sizes = { 100, 1000, 10000, 100000, 1000000 };
    vector<int32_t> thread_num = { 1, 2, 5, 10, 20, 50, 100 };
    vector<vector<std::chrono::nanoseconds>> calculated_time(sizes.size(),
        vector<std::chrono::nanoseconds>(thread_num.size()));

    for (int s = 0; s < sizes.size(); s++)
    {
        size_t n = sizes[s];

        for (int k = 0; k < thread_num.size(); k++)
        {
            list<int32_t> li;
            vector<thread*> threads;
            int m = thread_num[k],
                part = n / m;
            barrier insert_barr(m);

            //first insert values
            for (int i = 0; i < m; i++)
            {
                threads.push_back(new thread([&](int i_) {
                    insert_barr.arrive_and_wait();
                    for (int j = part * i_; j < part * (i_ + 1); j++)
                    {
                        if (j % 2 == 0)
                            li.push_back(j);
                        else
                            li.push_front(j);
                    }
                    }, i));
            }

            for (int i = 0; i < m; ++i) {
                threads[i]->join();
            }

            assert(li.size() == n, "Incorrect size of list");


            // then delete all elements using pop_back and pop_front
            threads.clear();
            barrier delete_barr(m);
            auto time_begin = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < m; i++)
            {
                threads.push_back(new thread([&](int i_) {
                    insert_barr.arrive_and_wait();
                    for (int j = 0; j < part; j++)
                    {
                        if (j % 2 == 0)
                            li.pop_back();
                        else
                            li.pop_front();
                    }
                    }, i));
            }

            for (int i = 0; i < m; ++i) {
                threads[i]->join();
            }

            //save time
            auto time_end = std::chrono::high_resolution_clock::now();
            auto time_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(time_end - time_begin);
            calculated_time[s][k] = time_elapsed;

            // check all elements were deleted
            assert(li.empty(), "list with n elem. not empty after n pop_front");
        }

        cout << n << "  OK\n";
    }

    printCalculatedTime(calculated_time, sizes, thread_num);
    cout << "Test Two OK\n" << "______________________________\n\n";
}

void Test_Three()
{
    size_t n = 10000;
    vector<size_t> sizes = { 100, 1000, 10000, 100000, 1000000 };
    vector<int32_t> thread_num = { 1, 2, 5, 10, 20, 50, 100 };
    vector<vector<std::chrono::nanoseconds>> calculated_time(sizes.size(),
        vector<std::chrono::nanoseconds>(thread_num.size()));

    for (int s = 0; s < sizes.size(); s++)
    {
        size_t n = sizes[s];

        for (int k = 0; k < thread_num.size(); k++)
        {
            list<int32_t> li;
            vector<thread*> threads;
            int m = thread_num[k],
                part = n / m;
            barrier insert_barr(m);

            //first insert values  (-j)
            for (int i = 0; i < m; i++)
            {
                threads.push_back(new thread([&](int i_) {
                    insert_barr.arrive_and_wait();
                    for (int j = part * i_; j < part * (i_ + 1); j++)
                    {
                        if (j % 2 == 0)
                            li.push_back(-j);
                        else
                            li.push_front(-j);
                    }
                    }, i));
            }

            for (int i = 0; i < m; ++i) {
                threads[i]->join();
            }

            assert(li.size() == n, "Incorrect size of list");


            // go by iter and erase/insert some data 
            threads.clear();
            barrier delete_barr(m);
            auto time_begin = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < m; i++)
            {
                threads.push_back(new thread([&](int i_) {
                    delete_barr.arrive_and_wait();

                    if (i_ % 2 == 0)
                    {
                        // go from begin to end

                        int k = 1;
                        int32_t inserted_value = 7;
                        atomic_int i = 1;
                        for (auto it = li.begin(); it != li.end(); it++)
                        {
                            if (k % 6 == 0)
                            {
                                li.erase(it);
                            }
                            else if (k % 10 == 0)
                            {
                                li.insert(it, inserted_value);
                                inserted_value += 7;
                            }

                            if (it == li.end())
                                break;

                            k++;
                            i++;
                        }
                    }
                    else
                    {
                        // go from end to begin
                        int k = 1;
                        int32_t inserted_value = 7;
                        atomic_int i = 1;
                        for (auto it = li.rend(); it != li.rbegin(); it--)
                        {
                            if (k % 6 == 0)
                            {
                                li.erase(it);
                            }
                            else if (k % 10 == 0)
                            {
                                li.insert(it, inserted_value);

                                inserted_value += 7;
                            }

                            if (it == li.end())
                                break;

                            k++;
                            i++;
                        }
                    }
                    }, i));
            }

            for (int i = 0; i < m; ++i) {
                threads[i]->join();
            }

            //save time
            auto time_end = std::chrono::high_resolution_clock::now();
            auto time_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(time_end - time_begin);
            calculated_time[s][k] = time_elapsed;


            // check: all inserted values must be --> val % 7 == 0
            if (!li.empty())
            {
                for (auto it = li.begin(); it != li.end(); it++)
                {
                    if (*it > 0 && (*it % 7 != 0))
                    {
                        assert(false, "Incorrect inserted values");
                    }
                }
            }
        }

        cout << n << "  OK\n";
    }

    printCalculatedTime(calculated_time, sizes, thread_num);
    cout << "Test Three OK\n" << "______________________________\n\n";
}


int main()
{
    Test_One();   // just insert
    Test_Two();   // just erase (calculate only erase)
    Test_Three(); // first insert then go by iter and erase/insert some data 

    return 0;
}
