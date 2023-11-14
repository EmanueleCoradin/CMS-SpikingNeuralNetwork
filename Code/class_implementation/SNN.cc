#include "SNN.h"
#include "Snnt_constants.h"


#include "TRandom.h"
#include "TRandom3.h"

#include "TCanvas.h"
#include "TGraph.h"
#include "TAxis.h"
#include "TF1.h"

using namespace std;


SNN::SNN(int NL0, int NL1): //Initializations
                            alpha(0.25),

                            CFI0(1),
                            CFI1(1),
                            CF01(1),

                            L1inhibitfactor(1),

                            K(1.),
                            K1(2.),
                            K2(4.),

                            IE_Pot_const(1),

                            IPSP_dt_dilation(1.),

                            MaxDelay(0.1e-9),

                            tau_m(1e-09),
                            tau_s(0.25e-09),
                            tau_plus(1.68e-09),
                            tau_minus(3.37e-09),

                            a_plus(0.00003125),
                            a_minus(0.00002656),

                            N_neurons(NL0 + NL1),
                            N_streams(N_InputStreams + NL0),
                            MaxFactor(0.2)
{
    Threshold[0] = 10;
    Threshold[1] = 10;


    N_neuronsL[0] = NL0;
    N_neuronsL[1] = NL1;


    tmax = tau_s * tau_m / (tau_m - tau_s) * (log(tau_m) - log(tau_s));

    MaxDeltaT = 7. * tau_m;

    
    //Print all the variables
    cout << "alpha = " << alpha << endl;
    cout << "CFI0 = " << CFI0 << endl;
    cout << "CFI1 = " << CFI1 << endl;
    cout << "CF01 = " << CF01 << endl;
    cout << "L1inhibitfactor = " << L1inhibitfactor << endl;
    cout << "K = " << K << endl;
    cout << "K1 = " << K1 << endl;
    cout << "K2 = " << K2 << endl;
    cout << "IE_Pot_const = " << IE_Pot_const << endl;
    cout << "IPSP_dt_dilation = " << IPSP_dt_dilation << endl;
    cout << "MaxDelay = " << MaxDelay << endl;
    cout << "tau_m = " << tau_m << endl;
    cout << "tau_s = " << tau_s << endl;
    cout << "tau_plus = " << tau_plus << endl;
    cout << "tau_minus = " << tau_minus << endl;
    cout << "a_plus = " << a_plus << endl;
    cout << "a_minus = " << a_minus << endl;
    cout << "N_neurons = " << N_neurons << endl;
    cout << "N_streams = " << N_streams << endl;
    cout << "MaxFactor = " << MaxFactor << endl;
    cout << "Threshold[0] = " << Threshold[0] << endl;
    cout << "Threshold[1] = " << Threshold[1] << endl;
    cout << "tmax = " << tmax << endl;
    cout << "MaxDeltaT = " << MaxDeltaT << endl;
    cout << "N_neuronsL[0] = " << N_neuronsL[0] << endl;
    cout << "N_neuronsL[1] = " << N_neuronsL[1] << endl;
    cout << "N_InputStreams = " << N_InputStreams << endl;
    cout << "NROOT = " << NROOT << endl;
    cout << "MaxClasses = " << MaxClasses << endl;
    cout << "BGR = " << BGR << endl;
    cout << "SIG = " << SIG << endl;
    cout << "MaxNeurons = " << MaxNeurons << endl;
    cout << "largenumber = " << largenumber << endl;
    cout << "epsilon = " << epsilon << endl;
    cout << "MaxEvents = " << MaxEvents << endl;
    cout << "MaxDeltaT = " << MaxDeltaT << endl;


    Init_neurons();
    Init_weights();
    Init_connection_map();

    cout<<"--------------------------"<<endl;
    //print the avarage weight, connection map and the initial weights
    cout << "Average weight = " << sumweight[0] << endl;
    cout << "Connection map = " << Void_weight[0][0] << endl;
    cout << "Initial weights = " << Weight_initial[0][0] << endl;


    
}

SNN::~SNN()
{
}

// Initialize neuron potentials
// ----------------------------
void SNN::Init_neurons()
{
    TRandom3 *myRNG = new TRandom3(23);
    for (int in = 0; in < N_neurons; in++)
    {
        // Set first event in history of this neuron
        History_time[in].push_back(0.);
        History_type[in].push_back(0);
        History_ID[in].push_back(0);
        if (in < N_neuronsL[0])
            Neuron_layer[in] = 0;
        else
            Neuron_layer[in] = 1;
    }
    return;
}

// Initialize synapse weights
// --------------------------
void SNN::Init_weights()
{
    TRandom3 *myRNG = new TRandom3(23);

    for (int in = 0; in < N_neurons; in++)
    {
        for (int is = 0; is < N_streams; is++)
        {
            check_LTD[in][is] = true; // flags used to see if we need to create a LTD signal after a neuron discharge
            Weight[in][is] = myRNG->Uniform();
            if (!Void_weight[in][is])
                sumweight[in] += Weight[in][is];
        }
    }

    for (int in = 0; in < N_neurons; in++)
    {
        for (int is = 0; is < N_streams; is++)
        {
            if (sumweight[in] > 0)
            {
                Weight[in][is] = Weight[in][is] / sumweight[in];
                Weight_initial[in][is] = Weight[in][is];
                OldWeight[in][is] = Weight[in][is]; // this will be used for the renorm
            }
        }
    }

    return;
}

// Initialize connection map
// -------------------------
void SNN::Init_connection_map()
{
    TRandom3 *myRNG = new TRandom3(23);

    // Setting L0 input connections
    for (int in = 0; in < N_neuronsL[0]; in++)
    {
        // input connections Tracking layers -> L0
        for (int is = 0; is < N_InputStreams; is++)
        {
            Void_weight[in][is] = false;
            if (myRNG->Uniform() > CFI0)
                Void_weight[in][is] = true;
        }
        // input connections L0 -> L0
        for (int is = N_InputStreams; is < N_streams; is++)
            Void_weight[in][is] = false;
    }

    // Setting L1 input connections
    for (int in = N_neuronsL[0]; in < N_neurons; in++)
    {
        // input connections Tracking layers -> L1
        for (int is = 0; is < N_InputStreams; is++)
        {
            Void_weight[in][is] = false;
            if (myRNG->Uniform() > CFI1)
                Void_weight[in][is] = true;
        }
        // input connections L0 -> L1
        for (int is = N_InputStreams; is < N_streams; is++)
        {
            Void_weight[in][is] = false;
            if (myRNG->Uniform() > CF01)
                Void_weight[in][is] = true;
        }
    }
    return;
}

// Model Excitatory Post-Synaptic Potential
// We take this as parametrized in T. Masquelier et al., "Competitive STDP-Based Spike Pattern Learning", DOI: 10.1162/neco.2008.06-08-804
// ---------------------------------------------------------------------------------------------------------------------------------------
float SNN::EPS_potential(float delta_t)
{
    float psp = 0.;
    if (delta_t >= 0. && delta_t < MaxDeltaT)
        psp = K * (exp(-delta_t / tau_m) - exp(-delta_t / tau_s));
    return psp;
}

// Model membrane potential after spike
// Also modeled as in paper cited above, like IPSP and other signals below
// -----------------------------------------------------------------------
float SNN::Spike_potential(float delta_t, int ilayer)
{
    float sp = 0.;
    if (delta_t >= 0. && delta_t < MaxDeltaT)
        sp = Threshold[ilayer] * (K1 * exp(-delta_t / tau_m) - K2 * (exp(-delta_t / tau_m) - exp(-delta_t / tau_s)));
    return sp;
}

// Model Inhibitory Post-Synaptic Potential (IPSP)
// -----------------------------------------------
float SNN::Inhibitory_potential(float delta_t, int ilayer)
{
    // In order to dilate the inhibition to larger times (in the attempt at obtaining higher selectivity),
    // we kludge it by multiplying delta_t and maxdeltat by a factor
    delta_t = delta_t * IPSP_dt_dilation;
    float ip = 0.;
    float thisalpha = alpha;
    if (ilayer > 0)
        thisalpha = L1inhibitfactor * alpha; // Different inhibition in L1
    if (delta_t >= 0. && delta_t < MaxDeltaT)
        ip = -thisalpha * Threshold[ilayer] * EPS_potential(delta_t);
    return ip;
}

// Compute collective effect of excitatory, post-spike, and inhibitory potentials on a neuron
// ------------------------------------------------------------------------------------------
float SNN::Neuron_firetime(int in, float t)
{
    int ilayer = Neuron_layer[in];
    float P0 = 0.;
    float t0 = History_time[in][0];
    float delta_t = t - t0;
    if (t0 > 0. && delta_t >= 0. && delta_t < MaxDeltaT)
    {
        int ilayer = Neuron_layer[in];
        P0 = Spike_potential(delta_t, ilayer); // the first event in the history sequence is a spike
    }
    float P = P0;

    // Now we extrapolate the effect of all past spikes and inhibitions to time t, to compute the potential when EPSP arrives
    int len = History_time[in].size();
    if (len > 1)
    {
        for (int ih = 1; ih < len - 1; ih++)
        {
            delta_t = t - History_time[in][ih];
            if (History_type[in][ih] == 1)
            { // EPSP
                if (delta_t > MaxDeltaT)
                { // Get rid of irrelevant events
                    History_time[in].erase(History_time[in].begin() + ih, History_time[in].begin() + ih + 1);
                    History_type[in].erase(History_type[in].begin() + ih, History_type[in].begin() + ih + 1);
                    History_ID[in].erase(History_ID[in].begin() + ih, History_ID[in].begin() + ih + 1);
                    len = len - 1;
                }
                else
                {
                    if (!Void_weight[in][History_ID[in][ih]]) // for type 1 or 3 signals, ID is the stream
                        P += Weight[in][History_ID[in][ih]] * EPS_potential(delta_t);
                }
            }
            else if (History_type[in][ih] == 2)
            { // IPSP
                if (delta_t > MaxDeltaT)
                { // get rid of irrelevant events
                    History_time[in].erase(History_time[in].begin() + ih, History_time[in].begin() + ih + 1);
                    History_type[in].erase(History_type[in].begin() + ih, History_type[in].begin() + ih + 1);
                    History_ID[in].erase(History_ID[in].begin() + ih, History_ID[in].begin() + ih + 1);
                    len = len - 1;
                }
                else
                {
                    int ilayer = Neuron_layer[in];
                    P += Inhibitory_potential(delta_t, ilayer);
                }
            }
            else if (History_type[in][ih] == 3)
            { // IE
                if (delta_t > MaxDeltaT)
                { // get rid of irrelevant events
                    History_time[in].erase(History_time[in].begin() + ih, History_time[in].begin() + ih + 1);
                    History_type[in].erase(History_type[in].begin() + ih, History_type[in].begin() + ih + 1);
                    History_ID[in].erase(History_ID[in].begin() + ih, History_ID[in].begin() + ih + 1);
                    len = len - 1;
                }
                else
                {
                    if (!Void_weight[in][History_ID[in][ih]]) // for type 1 or 3 signals, ID is the stream
                        P += IE_potential(delta_t, in, History_ID[in][ih]);
                }
            }
        }
    }
    if (P > Threshold[ilayer])
    { // Neuron will fire as spike contribution will bring it above threshold
        // compute fire time by looping more finely from t to t+tmax (tmax is peak time of EPSP)
        float this_t = t;
        do
        {
            P = P0;
            for (int ih = 1; ih < len; ih++)
            {
                float delta_t = this_t - History_time[in][ih];
                if (History_type[in][ih] == 1)
                { // EPSP
                    if (!Void_weight[in][History_ID[in][ih]])
                        P += Weight[in][History_ID[in][ih]] * EPS_potential(delta_t);
                }
                else if (History_type[in][ih] == 2)
                { // IPSP
                    int ilayer = Neuron_layer[in];
                    P += Inhibitory_potential(delta_t, ilayer);
                }
                else if (History_type[in][ih] == 3)
                { // IE
                    if (!Void_weight[in][History_ID[in][ih]])
                        P += IE_potential(delta_t, in, History_ID[in][ih]);
                }
            }
            this_t += 1 / (10000. * omega);
        } while (P < Threshold[ilayer] && this_t <= t + tmax);
        if (P >= Threshold[ilayer])
            return this_t;
    }
    return largenumber;
}

// Model Inhibitory-Excitatory signal (IE) as combination of two EPSP-like shapes, a negative one followed by a positive one
// This is a crude model, loosely inspired by shapes in Fig.2 of F. Sandin and M. Nilsson, "Synaptic Delays for Insect-Inspired
// Feature Detection in Dynamic Neuromorphic Processors", doi.org/10.3389/fnins.2020.00150
// ----------------------------------------------------------------------------------------------------------------------------
float SNN::IE_potential(float delta_t, int in, int is)
{
    float sp = 0.;
    if (delta_t >= 0. && delta_t < Delay[in][is])
    {
        sp = -IE_Pot_const * EPS_potential(delta_t);
    }
    else if (delta_t >= Delay[in][is] && delta_t < MaxDeltaT + Delay[in][is])
    {
        delta_t = delta_t - Delay[in][is];
        sp = IE_Pot_const * EPS_potential(delta_t); // So for zero delay, this is an EPSP
    }
    return sp;
}