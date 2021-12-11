#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>

using namespace std;

class PRDS_LRU
{
public:
    PRDS_LRU(int pages) { max = pages; }
    deque<int> pages;

    int push(int page)
    {

        auto recent = find(pages.begin(), pages.end(), page);
        if (recent != pages.end())
        {
            pages.erase(recent);
            pages.push_back(page);
            return -2;
        }
        else if (pages.size() < max)
        {
            pages.push_back(page);
            return -1;
        }
        else
        {
            int result = pages.front();
            pages.pop_front();
            pages.push_back(page);
            return result;
        }
    }

    int pop()
    {
        if (!pages.empty())
        {
            int page = pages.front();
            pages.pop_front();
            pages.push_back(page);
            return page;
        }
        else
        {
            cout << "Tried to pop empty queue in LRU" << endl; // Testing
            return -1;
        }
    }

private:
    int max;
};

int Page_Replacement_LRU(vector<int> &pages, int nextpage, PRDS_LRU *p)
{
    int replace = p->push(nextpage);
    for (int i = 0; i < pages.size(); i++)
    {
        if (pages[i] == replace)
        {
            return i;
        }
    }

    return -1;
}