#pragma once

#include "ListPage.h"

#include "ui/model/RouteListModel.h"

#include "model/Routing.h"

#include "engine/MidiLearn.h"

class RoutingPage : public ListPage {
public:
    RoutingPage(PageManager &manager, PageContext &context);

    using ListPage::show;
    void show(int routeIndex, const Routing::Route *initialValue = nullptr);

    virtual void enter() override;
    virtual void exit() override;

    virtual void draw(Canvas &canvas) override;

    virtual void keyPress(KeyPressEvent &event) override;
    virtual void encoder(EncoderEvent &event) override;

private:
    void showRoute(int routeIndex, const Routing::Route *initialValue = nullptr);
    void selectRoute(int routeIndex);
    void assignMidiLearn(const MidiLearn::Result &result);

    RouteListModel _routeListModel;
    Routing::Route *_route;
    uint8_t _routeIndex;
    Routing::Route _editRoute;
};
