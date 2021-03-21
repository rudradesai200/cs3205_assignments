#include <bits/stdc++.h>

/** Some defs for easy access */
#define pii pair<int, int>
#define piii pair<pii, int>
#define XX first
#define YY second
#define XYY first.first
#define YXY first.second
#define MP make_pair
#define YYX second

/** Hyperparams */
#define TIMEOUT 5 // Defines the interval timedelta before timeout
#define RWS 1000  // Receiver Window Size
#define MSS 1     // Maximum Segment Size

/** Global Declarations */
int timer, log_level;
std::random_device rd{}; // use to seed the rng
std::mt19937 rng{rd()};  // rng
int update_cnt;

using namespace std;

/** Sender class. Sends data segments and receives ACKs.
 * Uses Go-back-N algorithm with individual ACKs and timeout timers.
 * Similar to TCP Tahoe but doesn't contain support for 3 duplicate ACKs.
*/
class Sender
{
private:
    int next_segment_to_send;
    int curr_window_start;
    int last_ack;
    int curr_ack;
    ofstream fp;
    queue<pii> segments_sent_time; // Queue containing timers for currently sent segments.
    double cws;                    // Congestion window size
    int threshold;
    enum State
    {
        slow_start,           // cws grows exponentially
        congestion_avoidance, // cws grows linearly
    } state;

    int Ki, KT;
    double Km, Kn, Kf;

public:
    /**
     * @brief Initializes Sender object.
     * @param fname Filename to store cws update,
     * @param i initial cws size,
     * @param m cws multiplier during exponential growth,
     * @param n cws multiplier during linear growth,
     * @param f cws multiplier when a timeout occurs,
     * @param T total segments to be sent,
     */
    Sender(string fname, int i = 1, double m = 1.0, double n = 1.0, double f = 0.1, int T = 100) : Ki(i), Km(m), Kn(n), Kf(f), KT(T)
    {
        curr_window_start = 1;
        next_segment_to_send = 1;
        last_ack = 0;
        cws = Ki * MSS;
        threshold = RWS + 1;
        curr_ack = 0;
        fp.open(fname);
        fp << to_string(cws) << endl;
    }

    /**
     * @brief Prints debug information based on log_level.
     * Includes all the variables of this class.
     */
    void print()
    {
        cout << "Sender : " << endl;
        cout << "\tNext segment to send : " << next_segment_to_send << endl;
        cout << "\tcurr window start : " << curr_window_start << endl;
        cout << "\tlast ack : " << last_ack << endl;
        cout << "\tcurr ack : " << curr_ack << endl;
        cout << "\tcws : " << cws << endl;
        cout << "\tthreshold : " << threshold << endl;
        cout << "\tstate : " << state << endl;
        cout << "\tCurrently sent segments : " << segments_sent_time.size() << endl;
    }

    /**
     * @brief Clears all currently running timeout timers.
     */
    void clear_segments_sent_time()
    {
        while (!segments_sent_time.empty())
            segments_sent_time.pop();
    }

    /**
     * @brief Creates and returns the next data segment to be sent.
     * @return new segment
     */
    piii send_next_segment()
    {

        piii p = {{next_segment_to_send++, ::timer}, 1};
        segments_sent_time.push(p.XX);
        p.YXY++;
        if (log_level > 1)
            cout << "\033[0;32m==INFO\033[0m \033[0;33m(sender)\033[0m : sending segment " << next_segment_to_send - 1 << endl;
        return p;
    }

    /**
     * @brief Accepts the incoming ACK and takes action accordingly.
     * @param segment_no (Packet number to be acknowledged.)
     */
    void recv_ack(int segment_no)
    {
        if (log_level > 1)
            cout << "\033[0;32m==INFO\033[0m \033[0;33m(sender)\033[0m : received ack " << Ki << endl;

        curr_ack = segment_no;

        if (curr_ack == last_ack + 1)
        {
            curr_window_start++;
            segments_sent_time.pop();
            last_ack = curr_ack;
        }
        else
        {
            string err = "Received incorrect acknowledgement : " + segment_no;
            err += "\n";
            throw err;
        }
        change_cws();
    }

    /**
     * @brief Checks for timeout and changes object variables accordingly.
     * Threshold -> (cws/2) & (cws -> max(1, Kf * cws))
     * state changes to slow_start.
     */
    void check_timeout()
    {
        if (::timer - segments_sent_time.front().YY > TIMEOUT)
        {
            threshold = int(cws / 2);
            cws = max(1.0, Kf * cws);
            curr_window_start = segments_sent_time.front().XX;
            next_segment_to_send = curr_window_start;
            state = slow_start;
            clear_segments_sent_time();
            last_ack = curr_window_start - 1;
            if (log_level > 1)
                cout << "\033[0;32m==INFO\033[0m \033[0;33m(sender)\033[0m : Timeout for segment " << curr_window_start << endl;
        }
    }

    /**
     * @brief Checks sender's readyness for sending new segments.
     *
     * @return true if data segment is to be sent
     * @return false if not data segments to send.
     */
    bool is_ready_to_send()
    {
        if ((next_segment_to_send <= KT) && (next_segment_to_send <= curr_window_start + int(cws)) && (next_segment_to_send >= curr_window_start))
            return true;
        return false;
    }

    /**
     * @brief Checks for various conditions and changes cws accordingly.
     * Note: state may also get modified with the cws update.
     */
    void change_cws()
    {
        if (segments_sent_time.front().YY - ::timer > TIMEOUT)
        {
            check_timeout();
        }
        else if (curr_ack == last_ack)
        {
            if (state == congestion_avoidance)
                cws = min(cws + Kn * MSS * double(MSS / cws), double(RWS));
            else
                cws = min(cws + Km * MSS, double(RWS));
        }
        if (int(cws) >= threshold)
            state = congestion_avoidance;
        else
            state = slow_start;

        fp << to_string(cws) << endl;
    }

    void clean()
    {
        fp.close();
    }
};

/** Receiver class. Receives data segments and sends ACKs.
 *  Send individual ACKs. Note: it discards out-of-order segments.
 */
class Receiver
{
private:
    int last_segment_ack;
    int next_ack_to_send;
    int KT;        // max segments to be received
    bool complete; // is transmission completed?

public:
    /** @brief Initializes Receiver object.
     * @param T Max segment to be received. Set by switch
    */
    Receiver(int T = 100) : KT(T)
    {
        last_segment_ack = 0;
        next_ack_to_send = -1;
        complete = false;
    }

    /** Checks if the last segment is transmitted or not. */
    bool is_complete() { return complete; }

    /** Returns receiver object's readiness to send ACK*/
    bool ready_to_send_ack()
    {
        if (next_ack_to_send == -1)
            return false;
        return true;
    }

    /** @brief Receive segment and take action accordingly.
     * @param p Received data segment
    */
    void recv_segment(pii p)
    {
        if (log_level > 1)
            cout << "\033[0;32m==INFO\033[0m \033[0;33m(receiver)\033[0m : Received segment " << p.XX << endl;
        if (p.XX != last_segment_ack + 1)
            next_ack_to_send = -1;
        else
            next_ack_to_send = last_segment_ack + 1;
    }

    /** @brief Send ACK based on the last received segment.
     * Note: use ready_to_send_ack() first.
     * @return Returns a 3 tuple <ack, timer, -1>
    */
    piii send_ack()
    {
        if (log_level > 1)
            cout << "\033[0;32m==INFO\033[0m \033[0;33m(receiver)\033[0m : Sending ack ";
        if (next_ack_to_send == KT)
            complete = true;
        if (next_ack_to_send != -1)
        {
            int temp = next_ack_to_send;
            next_ack_to_send = -1;
            last_segment_ack++;
            if (log_level > 1)
                cout << temp << endl;
            return MP(MP(temp, ::timer + 1), -1);
        }
        else
        {
            if (log_level > 1)
                cout << next_ack_to_send << endl;
            return MP(MP(next_ack_to_send, ::timer + 1), -1);
        }
    }

    /** Prints debug information */
    void print()
    {
        cout << "Receiver : " << endl;
        cout << "\tlast segment ack : " << last_segment_ack << endl;
        cout << "\tnext ack to send : " << next_ack_to_send << endl;
    }
};

/**
 * @brief Functor used for comparing 3 tuples representing data segment and ACK.
 * Used in priority queue, for creating a min heap.
 */
class mycomp
{
public:
    int operator()(const piii &p1, const piii &p2)
    {
        if (p1.YXY > p2.YXY)
            return true;
        else if (p1.YXY < p2.YXY)
            return false;
        else
        {
            if (p1.YYX < p2.YYX)
                return false;
            else if (p1.YYX > p2.YYX)
                return true;
            else
            {
                return p1.XYY > p2.XYY;
            }
        }
    }
};

/**
 * @brief Switch class implementation.
 * Helps in sending and receiving segments and ACKs between sender and receiver object.
 * Pulls data segment from sender object if ready and pushes it to the min heap.
 * Similarly, pulls ACK from receiver object if ready and pushes it to the min heap.
 * Also, checks for various corner cases like, timeout in sender and transmission complete in receiver.
 */
class Switch
{
    Sender *s;
    Receiver *r;
    double pr;                                    // Probability of successful segment transfer from this switch
    priority_queue<piii, vector<piii>, mycomp> q; // Min heap to store the 3 tuple representing data segment and ACK

public:
    /**
    * @brief Construct a new Switch object. Initializes Sender and receiver objects.
    * @param fname Filename to store cws updates of sender,
    * @param i initial cws size,
    * @param m cws multiplier during exponential growth,
    * @param n cws multiplier during linear growth,
    * @param f cws multiplier when a timeout occurs,
    * @param T total segments to be sent,
    * @param Ps Probability of succesfull segment tranfer from this switch
    */
    Switch(string fname, int i = 1, double m = 1.0, double n = 1.0, double f = 0.1, int T = 100, double Ps = 0.95) : pr(Ps)
    {
        s = new Sender(fname, i, m, n, f, T);
        r = new Receiver(T);
        if (log_level > 1)
        {
            cout << "\033[0;32m==INFO\033[0m \033[0;33m(switch)\033[0m : Ki set to " << i << endl;
            cout << "\033[0;32m==INFO\033[0m \033[0;33m(switch)\033[0m : Km set to " << m << endl;
            cout << "\033[0;32m==INFO\033[0m \033[0;33m(switch)\033[0m : Kn set to " << n << endl;
            cout << "\033[0;32m==INFO\033[0m \033[0;33m(switch)\033[0m : Kf set to " << f << endl;
            cout << "\033[0;32m==INFO\033[0m \033[0;33m(switch)\033[0m : Ps set to " << Ps << endl;
            cout << "\033[0;32m==INFO\033[0m \033[0;33m(switch)\033[0m : T set to " << T << endl;
        }
    }

    /**
     * @brief Prints debug information according to the log level
     */
    void print()
    {
        if (log_level > 0)
        {
            cout << "==========================================" << endl;
            cout << "Switch :" << endl;
            cout << "\ttimer : " << ::timer << endl;
            cout << "\tCurrent q length : " << q.size() << endl
                 << endl;
            s->print();
            cout << endl;
            r->print();
            cout << "==========================================" << endl;
        }
    }

    /**
     * @brief Simulates sending and receiving of segments and ACKS.
     */
    void simulate()
    {
        std::bernoulli_distribution d(pr);
        for (::timer = 0;; ::timer++)
        {

            s->check_timeout(); // Check sender for timeout

            /** Receive ACK if scheduled and push next segment to min heap accordingly */
            while ((!q.empty()) && (q.top().YYX < 0) && (q.top().YXY == ::timer))
            {
                s->recv_ack(q.top().XYY);
                q.pop();
                if (s->is_ready_to_send())
                {
                    piii p = s->send_next_segment();
                    if (d(rng) == 1) // Randomly decides if this packet is to be dropped or not
                        q.push(p);
                    else if (log_level > 1)
                        cout << "\033[0;32m==INFO\033[0m \033[0;33m(switch)\033[0m : Dropped segment " << p.XYY << endl;
                }
            }

            /** Receive segment if schedule and push next ACK to min heap accordingly */
            while ((!q.empty()) && (q.top().YYX > 0) && (q.top().YXY == ::timer))
            {
                r->recv_segment(q.top().XX);
                q.pop();
                if (r->ready_to_send_ack())
                    q.push(r->send_ack());
            }

            /** Check if sender has more segments to push */
            while (s->is_ready_to_send())
            {
                piii p = s->send_next_segment();
                if (d(rng) == 1) // Randomly decides if this packet is to be dropped or not
                    q.push(p);
                else if (log_level > 1)
                    cout << "\033[0;32m==INFO\033[0m \033[0;33m(switch)\033[0m : Dropped segment " << p.XYY << endl;
            }

            /** Check if receiver has more ACKs to push */
            while (r->ready_to_send_ack())
                q.push(r->send_ack());

            /** Print debug information based on log_level */
            print();

            /** Check receiver if all the segments are received */
            if (r->is_complete())
                break;
        }

        s->clean();
    }
};

/**
 * @brief Helper functions for commandline argument parsing
 */
class ArgParse
{
public:
    /**
     * @brief Get the Cmdline argument value
     *
     * @param begin start of array of strings to search in. Ex : argv
     * @param end end of array of strings to search in. Ex : argv
     * @param option argument to search for Ex: -h
     * @return char* (corresponding argument value)
     */
    static char *getCmdOption(char **begin, char **end, const std::string &option)
    {
        char **itr = std::find(begin, end, option);
        if (itr != end && ++itr != end)
        {
            return *itr;
        }
        return 0;
    }

    /**
     * @brief Get the Cmdline argument value
     *
     * @param begin start of array of strings to search in. Ex : argv
     * @param end end of array of strings to search in. Ex : argv
     * @param option argument to search for Ex: -h
     * @return True if argument exists else False
     */
    static bool cmdOptionExists(char **begin, char **end, const std::string &option)
    {
        return std::find(begin, end, option) != end;
    }

    /**
     * @brief Helps in printing arguments.
     *
     * @param option argument
     * @param help help string
     */
    static void print_options(string option, string help)
    {
        printf("\t%-15s%s\n", option.c_str(), help.c_str());
    }
};

/**
 * @brief Displays help section for this program
 */
void display_help()
{
    cout << "Usage: ./simulation -o <filename> [options]" << endl;
    ArgParse::print_options("-h, --help", "Shows the available options. Prints this output");
    ArgParse::print_options("-i", "Initial congestion window. Default : 1 (int)");
    ArgParse::print_options("-m", "Congestion Window multiplier during exponential growth phase. Default : 1.0 (float)");
    ArgParse::print_options("-n", "Congestion Window multiplier during linear growth phase. Default : 1.0 (float)");
    ArgParse::print_options("-f", "Congestion Window multiplier when a timeout occurs. Default : 0.1 (float)");
    ArgParse::print_options("-s", "Probability of receiving the ACK segment for a given segment. Default : 0.95 (float)");
    ArgParse::print_options("-T", "Number of segments to be sent before the emulation stops. Default : 100 (int)");
    ArgParse::print_options("-l", "Logging level for simulation. : 0 (int)");
    ArgParse::print_options("-o", "Output filename to store cws updates for plotting.");
}

int main(int argc, char **argv)
{
    // Initialize variables
    int i = 1, t = 100;
    float m = 1.0, n = 1.0, f = 0.1, s = 0.95;
    string fname;
    ::timer = 0;
    ::update_cnt = 0;
    ::log_level = 0;

    // Parse command line arguments
    if (ArgParse::cmdOptionExists(argv, argv + argc, "--help") || ArgParse::cmdOptionExists(argv, argv + argc, "-h"))
    {
        display_help();
        return 0;
    }
    if (ArgParse::cmdOptionExists(argv, argv + argc, "-i"))
        i = stoi(ArgParse::getCmdOption(argv, argv + argc, "-i"));
    if (ArgParse::cmdOptionExists(argv, argv + argc, "-t"))
        t = stoi(ArgParse::getCmdOption(argv, argv + argc, "-t"));
    if (ArgParse::cmdOptionExists(argv, argv + argc, "-l"))
        ::log_level = stoi(ArgParse::getCmdOption(argv, argv + argc, "-l"));
    if (ArgParse::cmdOptionExists(argv, argv + argc, "-m"))
        m = stof(ArgParse::getCmdOption(argv, argv + argc, "-m"));
    if (ArgParse::cmdOptionExists(argv, argv + argc, "-n"))
        n = stof(ArgParse::getCmdOption(argv, argv + argc, "-n"));
    if (ArgParse::cmdOptionExists(argv, argv + argc, "-f"))
        f = stof(ArgParse::getCmdOption(argv, argv + argc, "-f"));
    if (ArgParse::cmdOptionExists(argv, argv + argc, "-s"))
        s = stof(ArgParse::getCmdOption(argv, argv + argc, "-s"));
    if (ArgParse::cmdOptionExists(argv, argv + argc, "-o"))
        fname = ArgParse::getCmdOption(argv, argv + argc, "-o");
    else
    {
        printf("-o is a required argument. Check -h for more details. \n");
        return -1;
    }

    // Start simulation.
    Switch swtch(fname, i, m, n, f, t, s);
    swtch.simulate();
}
