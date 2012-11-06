#include <iostream>
#include "fecho.h"
#include <Accelerate/Accelerate.h>
#include <fstream>

using namespace Fecho;

ofstream pyCode;
dSFMTRand mtrnd;

typedef double prec;

//see http://books.nips.cc/papers/files/nips15/AA14.pdf
void narma10(vector<float> &input, vector<float> &narma) {
    narma.resize(input.size());
    for(int i=10; i < narma.size(); i++) {
        narma[i] = 0.3*narma[i-1] + 0.05*narma[i-1]*(narma[i-1]+narma[i-2]+narma[i-3]+narma[i-4]+narma[i-5]+narma[i-6]+narma[i-7]+narma[i-8]+narma[i-9]+narma[i-10]) + 1.5*input[i-10]*input[i-1] + 0.1;
    }
}

void sineWaveTest() {
    clock_t ts = clock();

    ActivationFunctionTanh<prec> resAct;
    Reservoir<prec> net(0, 20, &resAct);
    net.setInScale(0).setInShift(0).setNoise(0.000001);
    
    MultiStreamRecorder<prec> actsRecorder;
    actsRecorder.setChannels(net.getNRes());
    
    ActivationFunctionLinear<prec> roAct;
    ReadOut<prec> ro(net, 1, &roAct);
    ro.setMapInsToOuts(true);
    
    try {
        Initialiser<prec> netInit;
        netInit.setResRangeLow(-0.5).setResRangeHigh(0.5).setResConnectivity(0.1)
        .setSpectralRadius(0.9)
        .setInConnectivity(0.0).setInRangeLow(-1).setInRangeHigh(1)
        .setFbConnectivity(1.0).setFbRangeLow(-0.9).setFbRangeHigh(0.9)
        .init(net, ro);
        net.dump();
    } catch (Initialiser<prec>::EVException e) {
        cout << e.what() << endl;
    }
    
    
    
//    SimulatorLI<float> sim(net, ro, 0.9);
    Simulator<prec> sim(net, ro);
    
    vector<prec> ins;
    ins.push_back(1.0);
    
    vector<prec> trainIn, trainOut;
    int trainSize = 300;
    int runSize = 50;
    trainIn.resize(0);
    trainOut.resize(trainSize);
    vector<prec> testOut(runSize);

    vector<prec> sig(trainSize + runSize);
    
    for(int i=0; i < sig.size(); i++) {
        prec r = 0.5 *sin(i/4.0);
        //or something more complex...
//        float r = (0.5 *sin(i/4.0)) + (0.1 * sin(i/7.0)) + (0.3 *sin(i));
//        r = pow(r,3);
        sig[i] = r;
    }
    
    std::copy(sig.begin(), sig.begin() + trainSize, trainOut.begin());
    std::copy(sig.begin() + trainSize, sig.end(), testOut.begin());
    
    TrainerPseudoInverse<prec> tr(&sim, &ro, trainIn, trainOut, trainSize, 100);
    cout << "Training...\n";
    tr.train();
    
    cout << "Running trained ESN\n";
    vector<prec> res(runSize);
//    net.resetStates();
    for(int i=0; i < runSize; i++) {
        sim.simulate(&trainIn[0]);
        res[i] = ro.getOutputs()[0];
        actsRecorder.addFrame(net.getActivations());
    }
    
    cout << "processed in " << ((clock() - ts) / (double) CLOCKS_PER_SEC) << " secs\n";

    pyCode << "simIn = array([";
//    for(int i=0; i < runSize; i++) {
//        cout << trainIn[i] << ",";
//        pyCode << trainIn[i] << ",";
//    }
//    cout << endl;
    pyCode << "])\n";

    cout << "Result: \n";
    pyCode << "simOut = array([";
    for(int i=0; i < runSize; i++) {
        cout << res[i] << ",";
        pyCode << res[i] << ",";
    }
    pyCode << "])\n";
    cout << endl;
    
    
    pyCode << toPyCode("testOut", testOut);
    
    pyCode << actsRecorder.dumpToPyCode();
    
    cout << "MSE: " << MSE<prec>::calc(res, testOut) << endl;
    
}

void sineWaveTest2() {
    clock_t ts = clock();
    
    ActivationFunctionTanh<prec> resAct;
    Reservoir<prec> net(0, 25, &resAct);
    net.setInScale(0).setInShift(0).setNoise(1e-6);
    
    MultiStreamRecorder<prec> actsRecorder;
    actsRecorder.setChannels(net.getNRes());
    
    ActivationFunctionLinear<prec> roAct;
    ReadOut<prec> ro(net, 1, &roAct);
    ro.setMapInsToOuts(true);
    
    try {
        Initialiser<prec> netInit;
        netInit.setResRangeLow(-1.0).setResRangeHigh(1.0).setResConnectivity(0.2)
        .setSpectralRadius(0.85)
        .setInConnectivity(0.0).setInRangeLow(-1).setInRangeHigh(1)
        .setFbConnectivity(1.0).setFbRangeLow(-2.0).setFbRangeHigh(2.0)
        .init(net, ro);
        net.dump();
    } catch (Initialiser<prec>::EVException e) {
        cout << e.what() << endl;
    }
    
    
    
    SimulatorLI<prec> sim(net, ro, 0.9);
    
    vector<prec> ins;
    ins.push_back(1.0);
    
    vector<prec> trainIn, trainOut;
    int trainSize = 4000;
    int runSize = 200;
    trainIn.resize(0);
    trainOut.resize(trainSize);
    vector<prec> testOut(runSize);
    
    vector<prec> sig(trainSize + runSize);
    
    for(int i=0; i < sig.size(); i++) {
        prec r = sin(i/150.0) + sin(i/13.0);
        sig[i] = r;
    }
    
    std::copy(sig.begin(), sig.begin() + trainSize, trainOut.begin());
    std::copy(sig.begin() + trainSize, sig.end(), testOut.begin());
    
    TrainerPseudoInverse<prec> tr(&sim, &ro, trainIn, trainOut, trainSize, 2000);
    cout << "Training...\n";
    tr.train();
    
    cout << "Running trained ESN\n";
    vector<prec> res(runSize);
    //    net.resetStates();
    for(int i=0; i < runSize; i++) {
        sim.simulate(&trainIn[0]);
        res[i] = ro.getOutputs()[0];
        actsRecorder.addFrame(net.getActivations());
    }
    
    cout << "processed in " << ((clock() - ts) / (double) CLOCKS_PER_SEC) << " secs\n";
    
    pyCode << "simIn = array([";
    //    for(int i=0; i < runSize; i++) {
    //        cout << trainIn[i] << ",";
    //        pyCode << trainIn[i] << ",";
    //    }
    //    cout << endl;
    pyCode << "])\n";
    
    cout << "Result: \n";
    pyCode << "simOut = array([";
    for(int i=0; i < runSize; i++) {
        cout << res[i] << ",";
        pyCode << res[i] << ",";
    }
    pyCode << "])\n";
    cout << endl;
    
    
    pyCode << toPyCode("testOut", testOut);
    
    pyCode << actsRecorder.dumpToPyCode();
    
    cout << "MSE: " << MSE<prec>::calc(res, testOut) << endl;
    
}

int main(int argc, const char * argv[])
{
    pyCode.open("/tmp/esn.py", ios_base::out);
    sineWaveTest2();
    pyCode.close();
    
    
    
    //gels test
    
//    cout << endl << endl;
    //    float a[5*3] = {1,1,1,2,3,4,3,5,2,4,2,5,5,4,3};
    //    float b[5*2] = {-10,-3,12,14,14,12,16,16,18,16};
    //    
    //    vector<float> acm(15);
    //    Math::rowMajorToColMajor(a, 3, 5, &acm[0]);
    //    for(float &v: acm) {
    //        cout << v << ",";
    //    }
    //    cout << endl;
    //
    //    vector<float> bcm(10);
    //    Math::rowMajorToColMajor(b, 2, 5, &bcm[0]);
    //    for(float &v: bcm) {
    //        cout << v << ",";
    //    }
    //    cout << endl;
    //    
    //    char trans = 'N';
    //    int m=5;
    //    int n=3;
    //    int nrhs = 2;
    //    int lda = 5;
    //    int ldb = 5;
    //    
    //    int lwork = -1;
    //    float wkopt;
    //    int info;
    //    
    //    Math::gels(&trans, &m, &n, &nrhs, &acm[0], &lda, &bcm[0], &ldb, &wkopt, &lwork, &info);
    //    lwork = (int)wkopt;
    //    vector<float> work(lwork);
    //    Math::gels(&trans, &m, &n, &nrhs, &acm[0], &lda, &bcm[0], &ldb, &work[0], &lwork, &info);
    //    cout << "info: " << info << endl;
    //    for(float &v: bcm) {
    //        cout << v << ",";
    //    }
    //    cout << endl;
    //    
    //    vector<float> result(nrhs * n);
    //    for(int i=0; i < nrhs; i++) {
    //        for(int j=0; j < n; j++) {
    //            result[(i*n) + j] = bcm[(i*m) + j];
    //        }
    //    }
    //    for(float &v: result) {
    //        cout << v << ",";
    //    }
    //    cout << endl << "gelss:\n";
    
    //now try with gelss
    //re-init bcm
    //    Math::rowMajorToColMajor(b, 2, 5, &bcm[0]);
    //    for(float &v: bcm) {
    //        cout << v << ",";
    //    }
    //    cout << endl;
    //    vector<float> s(min(m,n));
    //    float rcond = 0.01;
    //    int rank;
    //    lwork = -1;
    //    Math::gelss(&m, &n, &nrhs, &acm[0], &lda, &bcm[0], &ldb, &s[0], &rcond, &rank, &wkopt, &lwork, &info);
    //    lwork = (int)wkopt;
    //    vector<float> work2(lwork);
    //    Math::gelss(&m, &n, &nrhs, &acm[0], &lda, &bcm[0], &ldb, &s[0], &rcond, &rank, &work2[0], &lwork, &info);
    //    cout << "info: " << info << endl;
    //    cout << "Rank: " << rank << endl;
    //    for(float &v: bcm) {
    //        cout << v << ",";
    //    }
    //    cout << endl;
    //    for(int i=0; i < nrhs; i++) {
    //        for(int j=0; j < n; j++) {
    //            result[(i*n) + j] = bcm[(i*m) + j];
    //        }
    //    }
    //    for(float &v: result) {
    //        cout << v << ",";
    //    }
    
    
    
    
    return 0;
}

