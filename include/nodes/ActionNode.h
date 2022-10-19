//
// Created by Xuefeng Huang on 2020/1/30.
//

#ifndef TEXASSOLVER_ACTIONNODE_H
#define TEXASSOLVER_ACTIONNODE_H


#include <trainable/Trainable.h>
#include <thread>
#include <mutex>
#include <ranges/PrivateCards.h>
#include "GameTreeNode.h"
#include "GameActions.h"

class ActionNode :public GameTreeNode {
public:
    ActionNode(vector<GameActions> actions, vector<shared_ptr<GameTreeNode>> childrens, int player, GameRound round,double pot,shared_ptr<GameTreeNode> parent);
    vector<GameActions>& getActions();
    vector<shared_ptr<GameTreeNode>>& getChildrens();
    int getPlayer();
    shared_ptr<Trainable> getTrainable(int i,bool create_on_site=true);
    void setTrainable(vector<shared_ptr<Trainable>> trainable,vector<PrivateCards>* player_privates);
    vector<PrivateCards>* player_privates;

private:
    GameTreeNodeType getType() override;
private:
    // TODO ������Լ���
    vector<GameActions> actions;
public:
    void setActions(const vector<GameActions> &actions);

    void setChildrens(const vector<shared_ptr<GameTreeNode>> &childrens);

private:
    // TODO ����Ҳ���Լ��ʣ���ͬchance node֮��Ľڵ㶼���Ը���,����ͨ����¼һ��id����ʽ���ֲ�ͬ��֧,���ã����ã����õ�����
    vector<shared_ptr<GameTreeNode>> childrens;
    vector<shared_ptr<Trainable>> trainables;
    int player;
};


#endif //TEXASSOLVER_ACTIONNODE_H
