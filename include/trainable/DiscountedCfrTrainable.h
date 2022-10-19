//
// Created by Xuefeng Huang on 2020/1/31.
//

#ifndef TEXASSOLVER_DISCOUNTEDCFRTRAINABLE_H
#define TEXASSOLVER_DISCOUNTEDCFRTRAINABLE_H
#include <nodes/ActionNode.h>
#include <ranges/PrivateCards.h>
#include "Trainable.h"
using namespace std;

class DiscountedCfrTrainable:public Trainable {
private:
    ActionNode& action_node;
    vector<PrivateCards>* privateCards;
    int action_number;
    int card_number;
    vector<float> r_plus;
    constexpr static float alpha = 1.5f;
    constexpr static float beta = 0.5f;
    constexpr static float gamma = 2;
    constexpr static float theta = 0.9f;
    // TODO �@�Y�ܲ��ܼ���
    vector<float> r_plus_sum;
    vector<float> cum_r_plus;
    //vector<float> cum_r_plus_sum;
    //vector<float> current_strategy;
    //vector<float> average_strategy;
public:
    DiscountedCfrTrainable(vector<PrivateCards> *privateCards,
                           ActionNode &actionNode);
    bool isAllZeros(const vector<float>& input_array);

    const vector<float> getAverageStrategy() override;

    const vector<float> getcurrentStrategy() override;

    void updateRegrets(const vector<float>& regrets, int iteration_number, const vector<float>& reach_probs) override;

    void copyStrategy(shared_ptr<Trainable> other_trainable);

    json dump_strategy(bool with_state) override;

private:

    const vector<float> getcurrentStrategyNoCache();

    TrainableType get_type() override;

};


#endif //TEXASSOLVER_DISCOUNTEDCFRTRAINABLE_H
