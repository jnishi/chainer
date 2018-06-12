#include "xchainer/routines/pooling.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <nonstd/optional.hpp>

#include "xchainer/array.h"
#include "xchainer/check_backward.h"
#include "xchainer/constant.h"
#include "xchainer/device_id.h"
#include "xchainer/shape.h"
#include "xchainer/stack_vector.h"
#include "xchainer/testing/array.h"
#include "xchainer/testing/array_check.h"
#include "xchainer/testing/device_session.h"

namespace xchainer {
namespace {

class PoolingTest : public ::testing::TestWithParam<std::string> {
protected:
    void SetUp() override {
        const std::string& backend_name = GetParam();
        device_session_.emplace(DeviceId{backend_name, 0});
    }

    void TearDown() override { device_session_.reset(); }

private:
    nonstd::optional<testing::DeviceSession> device_session_;
};

TEST_P(PoolingTest, MaxPool) {
    if (GetParam() == "cuda") {
        // CuDNN convolution does not support cover_all, which is true by default.
        return;
    }
    using T = float;

    int64_t batch_size = 3;
    int64_t channels = 4;
    Shape in_dims{4, 4};
    StackVector<int64_t, kMaxNdim> kernel_size{3, 2};
    StackVector<int64_t, kMaxNdim> stride{2, 1};
    StackVector<int64_t, kMaxNdim> pad{1, 0};
    Shape out_dims{3, 3};

    Shape x_shape{batch_size, channels};
    std::copy(in_dims.begin(), in_dims.end(), std::back_inserter(x_shape));
    Shape out_shape{batch_size, channels};
    std::copy(out_dims.begin(), out_dims.end(), std::back_inserter(out_shape));

    Array x = testing::BuildArray(x_shape)
                      .WithData<T>(
                              {0.4609564,  0.9677815,  0.08834644, 0.32941917, 0.8292747,  0.11845724, 0.1369321,  0.6074731,  0.78542763,
                               0.97462624, 0.7587014,  0.27017736, 0.7416107,  0.25967184, 0.34700692, 0.80568856, 0.7809916,  0.42632404,
                               0.00986403, 0.96131665, 0.3818825,  0.8825699,  0.32384965, 0.41834843, 0.04444144, 0.5976533,  0.13136163,
                               0.99313796, 0.20961371, 0.72154176, 0.8705839,  0.8913622,  0.10679037, 0.13024911, 0.29701585, 0.9328509,
                               0.29156446, 0.56731504, 0.08864314, 0.40091372, 0.77066797, 0.7060211,  0.09676889, 0.09575618, 0.8108767,
                               0.04177126, 0.04649203, 0.17163254, 0.49008262, 0.6410025,  0.8935332,  0.06818897, 0.49048236, 0.6969235,
                               0.03973121, 0.99195975, 0.92212546, 0.2568234,  0.74829006, 0.49562132, 0.73161113, 0.3562581,  0.09955528,
                               0.42987233, 0.07259833, 0.02391367, 0.6531855,  0.2526743,  0.96925163, 0.99820083, 0.35440677, 0.45040762,
                               0.8124794,  0.6873842,  0.3394505,  0.99982584, 0.37112013, 0.9059909,  0.12874502, 0.10643691, 0.21043272,
                               0.22249077, 0.2507038,  0.38035834, 0.76311666, 0.58646417, 0.73357195, 0.70151967, 0.82812095, 0.45903125,
                               0.854887,   0.9307932,  0.5138541,  0.00605829, 0.88109905, 0.05902579, 0.93474656, 0.516853,   0.80964804,
                               0.6165152,  0.8065471,  0.4231297,  0.8462578,  0.12397768, 0.96989137, 0.13212852, 0.39432606, 0.8906301,
                               0.54361,    0.05775663, 0.96815336, 0.44516703, 0.6066227,  0.10383689, 0.795366,   0.06057209, 0.8556079,
                               0.32239342, 0.9142884,  0.52067345, 0.33631396, 0.337069,   0.98927075, 0.45864356, 0.05180012, 0.6295072,
                               0.63463855, 0.99933624, 0.9241264,  0.2909103,  0.12271244, 0.43939343, 0.98111194, 0.82608557, 0.6107712,
                               0.08100884, 0.6419318,  0.80480385, 0.24884045, 0.6263302,  0.40993217, 0.6449191,  0.7088349,  0.02296176,
                               0.70677763, 0.7166788,  0.2855127,  0.39801753, 0.8171236,  0.23696144, 0.4529571,  0.5830564,  0.41618168,
                               0.6569938,  0.73607063, 0.55866545, 0.10323327, 0.10768154, 0.9575846,  0.81976444, 0.6253338,  0.1517685,
                               0.1641238,  0.94771904, 0.86659664, 0.0256371,  0.1406688,  0.107798,   0.2999732,  0.7015409,  0.7981461,
                               0.09489103, 0.8165871,  0.8357075,  0.09764841, 0.05153274, 0.8971699,  0.9327884,  0.32184523, 0.15035488,
                               0.29527086, 0.34706247, 0.08613685, 0.22496991, 0.28078404, 0.17121029, 0.4556634,  0.5025214,  0.7903231,
                               0.87756634, 0.3690981,  0.6356852})
                      .WithPadding(1);  // Randomly generated.

    Array out = MaxPool(x, kernel_size, stride, pad);  // cover_all should be true.

    Array e_out = testing::BuildArray(out_shape).WithData<T>(
            {0.9677815,  0.9677815,  0.6074731,  0.97462624, 0.97462624, 0.80568856, 0.7416107,  0.34700692, 0.80568856, 0.8825699,
             0.8825699,  0.96131665, 0.8825699,  0.8825699,  0.99313796, 0.72154176, 0.8705839,  0.8913622,  0.56731504, 0.56731504,
             0.9328509,  0.8108767,  0.7060211,  0.40091372, 0.8108767,  0.04649203, 0.17163254, 0.6969235,  0.8935332,  0.99195975,
             0.92212546, 0.74829006, 0.99195975, 0.73161113, 0.3562581,  0.42987233, 0.99820083, 0.99820083, 0.6531855,  0.99820083,
             0.99820083, 0.99982584, 0.9059909,  0.9059909,  0.12874502, 0.76311666, 0.73357195, 0.73357195, 0.82812095, 0.88109905,
             0.9307932,  0.5138541,  0.88109905, 0.88109905, 0.93474656, 0.8462578,  0.8462578,  0.96989137, 0.96815336, 0.96815336,
             0.54361,    0.96815336, 0.96815336, 0.8556079,  0.9142884,  0.9142884,  0.8556079,  0.98927075, 0.99933624, 0.6295072,
             0.63463855, 0.99933624, 0.98111194, 0.82608557, 0.6107712,  0.98111194, 0.82608557, 0.7088349,  0.6449191,  0.7088349,
             0.7088349,  0.8171236,  0.7166788,  0.5830564,  0.8171236,  0.9575846,  0.9575846,  0.10768154, 0.9575846,  0.9575846,
             0.86659664, 0.1641238,  0.94771904, 0.86659664, 0.8357075,  0.7981461,  0.8357075,  0.8357075,  0.09764841, 0.9327884,
             0.9327884,  0.32184523, 0.87756634, 0.87756634, 0.6356852,  0.87756634, 0.87756634, 0.6356852});  // Computed with Chainer.

    testing::ExpectEqual(e_out, out);
}

TEST_P(PoolingTest, MaxPoolNoCoverAll) {
    using T = float;

    int64_t batch_size = 3;
    int64_t channels = 4;
    Shape in_dims{4, 4};
    StackVector<int64_t, kMaxNdim> kernel_size{3, 2};
    StackVector<int64_t, kMaxNdim> stride{2, 1};
    StackVector<int64_t, kMaxNdim> pad{1, 0};
    Shape out_dims{2, 3};
    bool cover_all = false;

    Shape x_shape{batch_size, channels};
    std::copy(in_dims.begin(), in_dims.end(), std::back_inserter(x_shape));
    Shape out_shape{batch_size, channels};
    std::copy(out_dims.begin(), out_dims.end(), std::back_inserter(out_shape));
    Array x = testing::BuildArray(x_shape)
                      .WithData<T>(
                              {0.951628,   0.8341918,  0.5700014,  0.02573909, 0.10652946, 0.45143777, 0.12487986, 0.6040584,  0.7059066,
                               0.8674204,  0.89753157, 0.3271186,  0.22637007, 0.7894245,  0.9550997,  0.03499391, 0.6357232,  0.4714537,
                               0.26333022, 0.15744655, 0.69606817, 0.5913821,  0.1362648,  0.00700566, 0.6983082,  0.41985217, 0.19198065,
                               0.87712926, 0.01699107, 0.85048497, 0.6478966,  0.81732035, 0.47958362, 0.335237,   0.6713821,  0.1833262,
                               0.8953133,  0.8300278,  0.46769994, 0.76619476, 0.57752323, 0.60258865, 0.27085522, 0.79189676, 0.98663867,
                               0.50531614, 0.16972028, 0.9301859,  0.53940713, 0.42947277, 0.7620938,  0.4948149,  0.2600685,  0.8730976,
                               0.3494606,  0.9889337,  0.5368636,  0.4020234,  0.23665707, 0.41831595, 0.62009174, 0.7569111,  0.7489499,
                               0.60345465, 0.8451688,  0.84799254, 0.99623865, 0.0536505,  0.320729,   0.68655115, 0.9852334,  0.890243,
                               0.76959133, 0.3614867,  0.11742796, 0.7991817,  0.05568137, 0.22353998, 0.26920173, 0.5037702,  0.45541075,
                               0.45879447, 0.48008284, 0.57052517, 0.3782304,  0.637869,   0.45500278, 0.71749103, 0.9862718,  0.21877514,
                               0.10590941, 0.5953773,  0.46771872, 0.73789245, 0.4005024,  0.7518998,  0.9913527,  0.5310464,  0.4475842,
                               0.5483692,  0.965521,   0.8801182,  0.18907578, 0.95214474, 0.02703529, 0.51783687, 0.17790386, 0.40175965,
                               0.5297797,  0.7417257,  0.22830275, 0.5155725,  0.933218,   0.31846902, 0.9928533,  0.8593246,  0.8691987,
                               0.83518404, 0.69086736, 0.3735951,  0.65166473, 0.58173877, 0.8519384,  0.54010224, 0.03113064, 0.4510318,
                               0.674089,   0.76923084, 0.42310983, 0.31675196, 0.8791844,  0.01504437, 0.98128337, 0.8053975,  0.14322701,
                               0.9443598,  0.96856105, 0.46812293, 0.6314993,  0.6479647,  0.44749212, 0.9877724,  0.7250273,  0.49135047,
                               0.56493795, 0.6489228,  0.04269254, 0.20499802, 0.16736922, 0.7334596,  0.40343535, 0.06048108, 0.7591618,
                               0.63597775, 0.11817221, 0.2982908,  0.00329836, 0.27108955, 0.02329292, 0.69136006, 0.8659653,  0.24925236,
                               0.33170977, 0.02298746, 0.11057855, 0.06332088, 0.04107838, 0.86021507, 0.72832036, 0.44712546, 0.15952812,
                               0.44132948, 0.8370784,  0.46001586, 0.14595562, 0.18176174, 0.68951994, 0.37592548, 0.0262325,  0.40434295,
                               0.05052375, 0.05624698, 0.10016874, 0.9320143,  0.09351984, 0.53812116, 0.20279366, 0.22279656, 0.33266315,
                               0.8101899,  0.6632538,  0.64406633})
                      .WithPadding(1);  // Randomly generated.
    Array out = MaxPool(x, kernel_size, stride, pad, cover_all);

    Array e_out = testing::BuildArray(out_shape).WithData<T>(
            {0.951628,   0.8341918,  0.6040584,  0.8674204, 0.9550997,  0.9550997,  0.69606817, 0.5913821,  0.26333022, 0.85048497,
             0.85048497, 0.87712926, 0.8953133,  0.8300278, 0.76619476, 0.98663867, 0.8300278,  0.9301859,  0.8730976,  0.8730976,
             0.9889337,  0.8730976,  0.8730976,  0.9889337, 0.84799254, 0.99623865, 0.99623865, 0.76959133, 0.9852334,  0.9852334,
             0.637869,   0.637869,   0.71749103, 0.9862718, 0.73789245, 0.7518998,  0.9913527,  0.8801182,  0.95214474, 0.965521,
             0.8801182,  0.95214474, 0.933218,   0.9928533, 0.9928533,  0.8691987,  0.8519384,  0.8519384,  0.98128337, 0.8791844,
             0.9443598,  0.9877724,  0.9877724,  0.9443598, 0.7334596,  0.7334596,  0.40343535, 0.7591618,  0.7334596,  0.69136006,
             0.8659653,  0.33170977, 0.86021507, 0.8370784, 0.46001586, 0.86021507, 0.68951994, 0.37592548, 0.9320143,  0.8101899,
             0.8101899,  0.9320143});  // Computed with Chainer.

    testing::ExpectEqual(e_out, out);
}

TEST_P(PoolingTest, MaxPoolNdNoCoverAll) {
    using T = float;

    int64_t batch_size = 3;
    int64_t channels = 4;
    Shape in_dims{3, 4, 2};
    StackVector<int64_t, kMaxNdim> kernel_size{2, 3, 2};
    StackVector<int64_t, kMaxNdim> stride{2, 1, 1};
    StackVector<int64_t, kMaxNdim> pad{1, 0, 0};
    Shape out_dims{2, 2, 1};
    bool cover_all = false;

    Shape x_shape{batch_size, channels};
    std::copy(in_dims.begin(), in_dims.end(), std::back_inserter(x_shape));
    Shape out_shape{batch_size, channels};
    std::copy(out_dims.begin(), out_dims.end(), std::back_inserter(out_shape));
    Array x = testing::BuildArray(x_shape)
                      .WithData<T>(
                              {0.7443965,  0.466585,   0.89991057, 0.6011775,  0.44392627, 0.01212001, 0.6850019,  0.15243967, 0.8667199,
                               0.2442128,  0.67573136, 0.36785454, 0.09906764, 0.32006702, 0.2941192,  0.06906214, 0.4547633,  0.08570488,
                               0.9805615,  0.63515943, 0.9235444,  0.31874985, 0.2922623,  0.8424672,  0.45487508, 0.51142967, 0.56195277,
                               0.16178702, 0.1434154,  0.47948307, 0.2100758,  0.78299475, 0.8435794,  0.5551395,  0.04787049, 0.09631719,
                               0.6097444,  0.8657279,  0.5092905,  0.8237162,  0.05624367, 0.84043086, 0.38281146, 0.7083898,  0.62722075,
                               0.46664056, 0.09982199, 0.92381614, 0.31687415, 0.12311904, 0.28741708, 0.5918304,  0.28770503, 0.49405122,
                               0.09714395, 0.9132574,  0.17417444, 0.8113134,  0.78891706, 0.03142597, 0.21724015, 0.08166417, 0.36335734,
                               0.5147829,  0.90494955, 0.773586,   0.5918902,  0.76111,    0.80608964, 0.62303025, 0.00699759, 0.11845457,
                               0.43313235, 0.39260346, 0.40701154, 0.42705905, 0.8120061,  0.8751227,  0.02269243, 0.56490266, 0.08994114,
                               0.09752213, 0.03915177, 0.34548464, 0.00377442, 0.51944125, 0.51867104, 0.25035092, 0.798595,   0.32923198,
                               0.7179019,  0.41776162, 0.95809466, 0.8846349,  0.55489767, 0.8336697,  0.21122925, 0.00538924, 0.9620717,
                               0.62521833, 0.7878091,  0.61587733, 0.82317215, 0.9371119,  0.89358616, 0.9317833,  0.5795524,  0.31719476,
                               0.54578173, 0.5663842,  0.747692,   0.7661823,  0.5601869,  0.11197416, 0.62696815, 0.2306318,  0.2143333,
                               0.4452788,  0.36305067, 0.29281017, 0.3174274,  0.8824883,  0.9707816,  0.78411233, 0.39922863, 0.28274158,
                               0.06927383, 0.27824947, 0.22943017, 0.930709,   0.6542054,  0.7272992,  0.48424295, 0.84909654, 0.7025952,
                               0.6957209,  0.7256895,  0.04510942, 0.15335312, 0.13091847, 0.5440975,  0.91838336, 0.55018735, 0.90877193,
                               0.44614205, 0.16832489, 0.01301944, 0.7548129,  0.73603594, 0.3235329,  0.69175094, 0.01074168, 0.16911024,
                               0.5299289,  0.7601291,  0.1179313,  0.33019885, 0.7097442,  0.688629,   0.2579612,  0.0695068,  0.35770202,
                               0.7073006,  0.05640914, 0.33988833, 0.7262268,  0.07806166, 0.9277037,  0.9180163,  0.84556764, 0.6071359,
                               0.42290464, 0.84606487, 0.3651215,  0.81922746, 0.10985906, 0.17247076, 0.7533703,  0.49501455, 0.15220903,
                               0.99647135, 0.42380828, 0.6003359,  0.22504759, 0.875422,   0.58578616, 0.9366149,  0.11690569, 0.42420593,
                               0.5917886,  0.70683753, 0.8183978,  0.4756214,  0.66599804, 0.5966581,  0.7142323,  0.0447604,  0.7221806,
                               0.40729043, 0.74232286, 0.16300161, 0.49036905, 0.978541,   0.07254261, 0.6489491,  0.92357546, 0.00269184,
                               0.08422355, 0.02356908, 0.65819466, 0.68853575, 0.71612203, 0.56305516, 0.12719513, 0.7743897,  0.451732,
                               0.8025118,  0.12570524, 0.08991196, 0.6524047,  0.49495652, 0.40224484, 0.86370593, 0.08105256, 0.36082205,
                               0.754245,   0.6098171,  0.94351375, 0.9433489,  0.8337473,  0.9479238,  0.3690827,  0.19420643, 0.86682785,
                               0.699835,   0.82302284, 0.37547496, 0.45134047, 0.4901204,  0.5570143,  0.44694754, 0.04374322, 0.65735126,
                               0.49562973, 0.5227552,  0.73393095, 0.84942234, 0.7592201,  0.86141664, 0.533247,   0.09745267, 0.2624992,
                               0.4333376,  0.40258995, 0.51826745, 0.5248199,  0.78600687, 0.5420168,  0.81095344, 0.01150763, 0.07383808,
                               0.7823987,  0.328573,   0.24309944, 0.50480926, 0.26718357, 0.5933533,  0.82193506, 0.8797568,  0.5687586,
                               0.8594091,  0.82480854, 0.8240346,  0.7362367,  0.6144636,  0.1727837,  0.50839895, 0.6949624,  0.29187527,
                               0.49906296, 0.255307,   0.520505,   0.4900493,  0.03956361, 0.08103298, 0.64027756, 0.6599848,  0.5945138})
                      .WithPadding(1);  // Randomly generated.
    Array out = MaxPool(x, kernel_size, stride, pad, cover_all);

    Array e_out = testing::BuildArray(out_shape).WithData<T>(
            {0.89991057, 0.89991057, 0.9805615,  0.9805615,  0.56195277, 0.78299475, 0.8657279, 0.92381614, 0.5918304,  0.9132574,
             0.90494955, 0.80608964, 0.8751227,  0.8751227,  0.95809466, 0.95809466, 0.9620717, 0.9620717,  0.9317833,  0.7661823,
             0.9707816,  0.9707816,  0.930709,   0.91838336, 0.7548129,  0.7548129,  0.7601291, 0.9277037,  0.9180163,  0.84606487,
             0.99647135, 0.99647135, 0.7221806,  0.74232286, 0.978541,   0.978541,   0.8025118, 0.86370593, 0.94351375, 0.9479238,
             0.73393095, 0.84942234, 0.86141664, 0.81095344, 0.8797568,  0.8797568,  0.8240346, 0.6949624});  // Computed with Chainer.

    testing::ExpectEqual(e_out, out);
}

TEST_P(PoolingTest, MaxPoolBackward) {
    if (GetParam() == "cuda") {
        // TODO(hvy): Test CUDA when implemented.
        return;
    }
    using T = float;

    int64_t batch_size = 3;
    int64_t channels = 4;
    Shape in_dims{4, 4};
    StackVector<int64_t, kMaxNdim> kernel_size{3, 2};
    StackVector<int64_t, kMaxNdim> stride{2, 1};
    StackVector<int64_t, kMaxNdim> pad{1, 0};
    Shape out_dims{3, 3};

    Shape x_shape{batch_size, channels};
    std::copy(in_dims.begin(), in_dims.end(), std::back_inserter(x_shape));
    Shape out_shape{batch_size, channels};
    std::copy(out_dims.begin(), out_dims.end(), std::back_inserter(out_shape));

    Array x = (*testing::BuildArray(x_shape)
                        .WithData<T>(
                                {0.4609564,  0.9677815,  0.08834644, 0.32941917, 0.8292747,  0.11845724, 0.1369321,  0.6074731,  0.78542763,
                                 0.97462624, 0.7587014,  0.27017736, 0.7416107,  0.25967184, 0.34700692, 0.80568856, 0.7809916,  0.42632404,
                                 0.00986403, 0.96131665, 0.3818825,  0.8825699,  0.32384965, 0.41834843, 0.04444144, 0.5976533,  0.13136163,
                                 0.99313796, 0.20961371, 0.72154176, 0.8705839,  0.8913622,  0.10679037, 0.13024911, 0.29701585, 0.9328509,
                                 0.29156446, 0.56731504, 0.08864314, 0.40091372, 0.77066797, 0.7060211,  0.09676889, 0.09575618, 0.8108767,
                                 0.04177126, 0.04649203, 0.17163254, 0.49008262, 0.6410025,  0.8935332,  0.06818897, 0.49048236, 0.6969235,
                                 0.03973121, 0.99195975, 0.92212546, 0.2568234,  0.74829006, 0.49562132, 0.73161113, 0.3562581,  0.09955528,
                                 0.42987233, 0.07259833, 0.02391367, 0.6531855,  0.2526743,  0.96925163, 0.99820083, 0.35440677, 0.45040762,
                                 0.8124794,  0.6873842,  0.3394505,  0.99982584, 0.37112013, 0.9059909,  0.12874502, 0.10643691, 0.21043272,
                                 0.22249077, 0.2507038,  0.38035834, 0.76311666, 0.58646417, 0.73357195, 0.70151967, 0.82812095, 0.45903125,
                                 0.854887,   0.9307932,  0.5138541,  0.00605829, 0.88109905, 0.05902579, 0.93474656, 0.516853,   0.80964804,
                                 0.6165152,  0.8065471,  0.4231297,  0.8462578,  0.12397768, 0.96989137, 0.13212852, 0.39432606, 0.8906301,
                                 0.54361,    0.05775663, 0.96815336, 0.44516703, 0.6066227,  0.10383689, 0.795366,   0.06057209, 0.8556079,
                                 0.32239342, 0.9142884,  0.52067345, 0.33631396, 0.337069,   0.98927075, 0.45864356, 0.05180012, 0.6295072,
                                 0.63463855, 0.99933624, 0.9241264,  0.2909103,  0.12271244, 0.43939343, 0.98111194, 0.82608557, 0.6107712,
                                 0.08100884, 0.6419318,  0.80480385, 0.24884045, 0.6263302,  0.40993217, 0.6449191,  0.7088349,  0.02296176,
                                 0.70677763, 0.7166788,  0.2855127,  0.39801753, 0.8171236,  0.23696144, 0.4529571,  0.5830564,  0.41618168,
                                 0.6569938,  0.73607063, 0.55866545, 0.10323327, 0.10768154, 0.9575846,  0.81976444, 0.6253338,  0.1517685,
                                 0.1641238,  0.94771904, 0.86659664, 0.0256371,  0.1406688,  0.107798,   0.2999732,  0.7015409,  0.7981461,
                                 0.09489103, 0.8165871,  0.8357075,  0.09764841, 0.05153274, 0.8971699,  0.9327884,  0.32184523, 0.15035488,
                                 0.29527086, 0.34706247, 0.08613685, 0.22496991, 0.28078404, 0.17121029, 0.4556634,  0.5025214,  0.7903231,
                                 0.87756634, 0.3690981,  0.6356852})
                        .WithPadding(1))
                      .RequireGrad();  // Same values as the MaxPool test.

    Array go = testing::BuildArray(out_shape).WithLinearData(-0.1f, 0.1f).WithPadding(1);

    Array eps = Full(x.shape(), 1e-3f);

    CheckBackward(
            [&](const std::vector<Array>& xs) -> std::vector<Array> { return {MaxPool(xs[0], kernel_size, stride, pad)}; },
            {x},
            {go},
            {eps},
            1e-6,
            1e-3);
}

TEST_P(PoolingTest, MaxPoolDoubleBackward) {
    if (GetParam() == "cuda") {
        // TODO(hvy): Test CUDA when implemented.
        return;
    }
    using T = float;

    int64_t batch_size = 3;
    int64_t channels = 4;
    Shape in_dims{4, 4};
    StackVector<int64_t, kMaxNdim> kernel_size{3, 2};
    StackVector<int64_t, kMaxNdim> stride{2, 1};
    StackVector<int64_t, kMaxNdim> pad{1, 0};
    Shape out_dims{3, 3};

    Shape x_shape{batch_size, channels};
    std::copy(in_dims.begin(), in_dims.end(), std::back_inserter(x_shape));
    Shape out_shape{batch_size, channels};
    std::copy(out_dims.begin(), out_dims.end(), std::back_inserter(out_shape));

    Array x = (*testing::BuildArray(x_shape)
                        .WithData<T>(
                                {0.4609564,  0.9677815,  0.08834644, 0.32941917, 0.8292747,  0.11845724, 0.1369321,  0.6074731,  0.78542763,
                                 0.97462624, 0.7587014,  0.27017736, 0.7416107,  0.25967184, 0.34700692, 0.80568856, 0.7809916,  0.42632404,
                                 0.00986403, 0.96131665, 0.3818825,  0.8825699,  0.32384965, 0.41834843, 0.04444144, 0.5976533,  0.13136163,
                                 0.99313796, 0.20961371, 0.72154176, 0.8705839,  0.8913622,  0.10679037, 0.13024911, 0.29701585, 0.9328509,
                                 0.29156446, 0.56731504, 0.08864314, 0.40091372, 0.77066797, 0.7060211,  0.09676889, 0.09575618, 0.8108767,
                                 0.04177126, 0.04649203, 0.17163254, 0.49008262, 0.6410025,  0.8935332,  0.06818897, 0.49048236, 0.6969235,
                                 0.03973121, 0.99195975, 0.92212546, 0.2568234,  0.74829006, 0.49562132, 0.73161113, 0.3562581,  0.09955528,
                                 0.42987233, 0.07259833, 0.02391367, 0.6531855,  0.2526743,  0.96925163, 0.99820083, 0.35440677, 0.45040762,
                                 0.8124794,  0.6873842,  0.3394505,  0.99982584, 0.37112013, 0.9059909,  0.12874502, 0.10643691, 0.21043272,
                                 0.22249077, 0.2507038,  0.38035834, 0.76311666, 0.58646417, 0.73357195, 0.70151967, 0.82812095, 0.45903125,
                                 0.854887,   0.9307932,  0.5138541,  0.00605829, 0.88109905, 0.05902579, 0.93474656, 0.516853,   0.80964804,
                                 0.6165152,  0.8065471,  0.4231297,  0.8462578,  0.12397768, 0.96989137, 0.13212852, 0.39432606, 0.8906301,
                                 0.54361,    0.05775663, 0.96815336, 0.44516703, 0.6066227,  0.10383689, 0.795366,   0.06057209, 0.8556079,
                                 0.32239342, 0.9142884,  0.52067345, 0.33631396, 0.337069,   0.98927075, 0.45864356, 0.05180012, 0.6295072,
                                 0.63463855, 0.99933624, 0.9241264,  0.2909103,  0.12271244, 0.43939343, 0.98111194, 0.82608557, 0.6107712,
                                 0.08100884, 0.6419318,  0.80480385, 0.24884045, 0.6263302,  0.40993217, 0.6449191,  0.7088349,  0.02296176,
                                 0.70677763, 0.7166788,  0.2855127,  0.39801753, 0.8171236,  0.23696144, 0.4529571,  0.5830564,  0.41618168,
                                 0.6569938,  0.73607063, 0.55866545, 0.10323327, 0.10768154, 0.9575846,  0.81976444, 0.6253338,  0.1517685,
                                 0.1641238,  0.94771904, 0.86659664, 0.0256371,  0.1406688,  0.107798,   0.2999732,  0.7015409,  0.7981461,
                                 0.09489103, 0.8165871,  0.8357075,  0.09764841, 0.05153274, 0.8971699,  0.9327884,  0.32184523, 0.15035488,
                                 0.29527086, 0.34706247, 0.08613685, 0.22496991, 0.28078404, 0.17121029, 0.4556634,  0.5025214,  0.7903231,
                                 0.87756634, 0.3690981,  0.6356852})
                        .WithPadding(1))
                      .RequireGrad();  // Same values as the MaxPool test.

    Array go = (*testing::BuildArray(out_shape).WithLinearData(-0.1f, 0.1f).WithPadding(1)).RequireGrad();
    Array ggx = testing::BuildArray(x_shape).WithLinearData(-0.1f, 0.1f).WithPadding(1);

    Array x_eps = Full(x_shape, 1e-3f);
    Array go_eps = Full(out_shape, 1e-3f);

    CheckDoubleBackwardComputation(
            [&](const std::vector<Array>& xs) -> std::vector<Array> {
                Array y = MaxPool(xs[0], kernel_size, stride, pad);
                return {y * y};
            },
            {x},
            {go},
            {ggx},
            {x_eps, go_eps},
            1e-2,
            1e-3);
}

INSTANTIATE_TEST_CASE_P(
        ForEachBackend,
        PoolingTest,
        ::testing::Values(
#ifdef XCHAINER_ENABLE_CUDA
                std::string{"cuda"},
#endif  // XCHAINER_ENABLE_CUDA
                std::string{"native"}));

}  // namespace
}  // namespace xchainer
