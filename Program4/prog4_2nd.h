#include <vector>
#include <deque>
#include <iostream>

using namespace std;

class PRDS_2nd
{
public:
    PRDS_2nd(int pages) // Initialize queue to mirror pages vector
    {
        for (int i = 0; i < pages; i++)
            this->pages.push_back(make_pair(-1, 0));
    }

    // Takes in the next page as a parameter and returns the an integer to tell the page replacement function what to do.
    // result == -2: Do not replace anything
    //        >= -1: Replace page <result>
    int replaceWith(int page)
    {
        for (int i = 0; i < pages.size(); i++)
        {
            auto n = &pages[i];
            if (n->first == page) // If page is found in queue, do not replace anything
            {
                if (n->second < 3) // If the "chance" counter is less than 3, increment it
                    n->second++;
                return -2; // Return -2 to tell the page replacement function to not replace anything.
            }
        }

        pages.push_back(make_pair(page, 0)); // Add new page to the back of the queue

        // Remove element at the front of the queue and check if it has a "chance" counter at 0
        auto victim = pages.front();
        pages.pop_front();

        // If it doesn't, decrement its chance counter, add it to the queue, and take the next victim
        while (victim.second > 0)
        {
            victim.second--;
            pages.push_back(victim);
            victim = pages.front();
            pages.pop_front();
        }

        // Return the value of the victim to be replaced
        return victim.first;
    }

private:
    deque<pair<int, int>> pages;
};

int Page_Replacement_2nd(vector<int> &pages, int nextpage, PRDS_2nd *p)
{
    int replace = p->replaceWith(nextpage); // Find value of page to be replaced
    if (replace < -1)                       // If less than -1, do nothing.
        return -1;

    // Replace page in vector
    for (int i = 0; i < pages.size(); i++)
    {
        if (pages[i] == replace)
        {
            return i;
        }
    }

    return -1;
}