#include <bits/stdc++.h>
#define TIMEOUT 3
#define RWS 1000
#define MSS 1

using namespace std;

int time;

class Sender
{
    int next_window_start;
    int last_ack;
    int curr_ack;
    int ndup_ack;
    queue<int> packets_sent_time;
    double cws;
    int threshold;
    enum State
    {
        slow_start,
        congestion_avoidance,

    } state;

    int Ki,
        KT;
    double Km, Kn, Kf, Ps;

    Sender(int i = 1, double m = 1.0, double n = 1.0, double f = 0.5, double s = 0.9, int T = 100) : Ki(i),
                                                                                                     Km(m), Kn(n), Kf(f), Ps(s), KT(T)
    {
        next_window_start = 1;
        last_ack = 0;
        curr_ack = 0;
        ndup_ack = 0;
        cws = Ki * MSS;
        threshold = RWS + 1;
    }

    void clear_packets_sent_time()
    {
        while (!packets_sent_time.empty())
            packets_sent_time.pop();
    }

    int send_packet()
    {
        packets_sent_time.push(::time);
        return next_window_start;
    }

    int recv_ack(int i)
    {
        curr_ack = i;
        if (i > last_ack + 1)
            if (i == last_ack + 1) // ack proper
            {
                last_ack++;
                packets_sent_time.pop();
                ndup_ack = 0;
            }
            else if (i == last_ack) // packet lost or delayed
                ndup_ack++;
            else
            {
                cout << "ERROR: incorect ack received " << i << ". last_ack = " << last_ack << endl;
                exit(EXIT_FAILURE);
            }
        change_cws();
    }

    void change_cws()
    {
        if (packets_sent_time.front() - ::time > TIMEOUT)
        {
            threshold = ceil(cws / 2);
            cws = max(1.0, Kf * cws);
            next_window_start = last_ack + 1;
            state = slow_start;
            clear_packets_sent_time();
        }
        else if (ndup_ack == 3) // congestion problem
        {                       // Fast recovery
            threshold = int(cws / 2);
            cws /= 2;
            next_window_start = last_ack + 1;
            state = congestion_avoidance;
            clear_packets_sent_time();
        }
        else if (last_ack == curr_ack)
        {
            if (state == congestion_avoidance)
            {
                cws = min(cws + Kn * MSS * double(MSS / cws), double(RWS));
                next_window_start++;
            }
            else
            {
                cws = min(cws + Km * MSS, double(RWS));
                next_window_start++;
            }
        }
        send_packet();
        if (int(cws) >= threshold)
            state = congestion_avoidance;
        else
            state = slow_start;
    }
};

int main()
{
}
