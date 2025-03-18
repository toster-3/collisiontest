#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/System/Clock.hpp"
#include "SFML/System/Vector2.hpp"
#include "SFML/Window/Event.hpp"
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cstddef>
#include <imgui.h>
#include <imgui-SFML.h>
#include <vector>
#include <cmath>
#include <utility>

#define PI_OVER_180 0.01745329251994329576
float radians(float x)
{
    return x * PI_OVER_180;
}
void do_imgui(sf::RectangleShape &r1, sf::RectangleShape &r2);;

sf::Clock deltaClock;

struct Vec2 : public sf::Vector2f {
    float x;
    float y;

    Vec2(float x, float y) : x(x), y(y) {}
    Vec2(sf::Vector2f v) : x(v.x), y(v.y) {}

    Vec2 operator-(Vec2 other) const {
        return {x-other.x, y-other.y};
    }
    float dot(const Vec2& other) const { return x * other.x + y * other.y; }
    Vec2 perpendicular() const { return {-y, x}; }
    Vec2 normalized() const {
        float len = sqrtf(x * x + y * y);
        return {x / len, y / len};
    }
    Vec2 rotate(float angle)
    {
        float theta = radians(angle);
        return Vec2(x*cosf(theta) - y*sinf(theta), x*sinf(theta) + y*cosf(theta));
    }
};

struct Rct {
    float x = 0;
    float y = 0;
    float height = 0;
    float width = 0;
    float rotation = 0;

    Rct() = default;
    Rct(float x, float y, float h, float w, float r) : x(x), y(y), height(h), width(w), rotation(r) {}
    Rct(sf::RectangleShape r) {
        auto pos = r.getPosition();
        auto size = r.getSize();
        x = pos.x;
        y = pos.y;
        width = size.x;
        height = size.y;
        rotation = r.getRotation();
    }

    Vec2 endpoint()
    {
        float theta = radians(rotation);
        return {x + width*cosf(theta) - height*sinf(theta), y + width*sinf(theta) + height*cosf(theta)};
    }
    std::vector<Vec2> corners()
    {
        Vec2 pos = {x,y};
        float theta = radians(rotation);

        Vec2 c0 = pos;
        Vec2 c1 = {x + width*cosf(theta),y + width*sinf(theta)};
        Vec2 c2 = {x - height*sinf(theta), y + height*cosf(theta)};
        Vec2 c3 = endpoint();

        return std::vector{c0, c1, c2, c3};
    }
    void toRectShape(sf::RectangleShape &r)
    {
        r.setPosition({x,y});
        r.setSize({width, height});
        r.setRotation(rotation);
    }
    sf::RectangleShape toRectShape()
    {
        sf::RectangleShape ret;
        toRectShape(ret);
        return ret;
    }
};

struct Point : public sf::Drawable {
    float x, y;
    sf::Color col = sf::Color::White;

    Point(Vec2 v) : x(v.x), y(v.y) {}
    Point(Vec2 v, sf::Color c) : x(v.x), y(v.y), col(c) {}

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        sf::CircleShape c;
        c.setRadius(10);
        auto lb = c.getLocalBounds().getSize();
        c.setPosition(x - lb.x/2.f, y - lb.y/2.f);
        // cool 3d effect
        //c.setPosition(x - lb.x/2.f, y/2.f);
        c.setFillColor(col);

        target.draw(c, states);
    }
};


std::vector<Vec2> getRotatedCorners(const sf::RectangleShape &r)
{
    float theta = radians(r.getRotation());
    Vec2 pos = r.getPosition();
    Vec2 size = r.getSize();

    std::vector<Vec2> corners = {
        pos,
        {pos.x + size.x*cosf(theta),pos.y + size.x*sinf(theta)},
        {pos.x - size.y*sinf(theta), pos.y + size.y*cosf(theta)},
        {pos.x + size.x*cosf(theta) - size.y*sinf(theta), pos.y + size.x*sinf(theta) + size.y*cosf(theta)},
    };

    return corners;
}

std::pair<float, float> projectOntoAxis(const std::vector<Vec2>& points, const Vec2& axis)
{
    float dot = points[0].dot(axis);
    float min = points[0].dot(axis), max = points[0].dot(axis);
    for (size_t i = 1; i < points.size(); i++) {
        dot = points[i].dot(axis);
        min = std::min(dot, min);
        max = std::max(dot, min);
    }

    return {min, max};
}

bool collides(const sf::RectangleShape &r1, const sf::RectangleShape &r2)
{
    auto corners1 = getRotatedCorners(r1);
    auto corners2 = getRotatedCorners(r2);

    std::vector<Vec2> axes = {
        corners1[1] - corners1[0],
        corners1[2] - corners1[1],
        corners1[3] - corners1[2],
        corners1[0] - corners1[3],

        corners2[1] - corners2[0],
        corners2[2] - corners2[1],
        corners2[3] - corners2[2],
        corners2[0] - corners2[3],
    };

    for (const Vec2& axis : axes) {
        auto minmax1 = projectOntoAxis(corners1, axis);
        auto minmax2 = projectOntoAxis(corners2, axis);

        if (minmax1.second < minmax2.first || minmax2.second < minmax2.first) {
            return false;
        }
    }

    return true;
}

sf::RenderWindow window;
Rct rc1 = {100,100,100,100, 45};
Rct rc2 = {300, 120, 60, 120, 0};
bool colliding = false;

void draw_corners(sf::RectangleShape &r)
{
    Rct rc(r);
    auto vc = rc.corners();
    auto v = getRotatedCorners(r);
    for (unsigned long i = 0; i < v.size(); i++) {
        window.draw(Point(vc[i], sf::Color::Blue));
        window.draw(Point(v[i], sf::Color::White));
    }
}

int main()
{
    sf::RectangleShape r1;
    r1.setSize({100, 100});
    r1.setPosition({100, 100});
    r1.setFillColor(sf::Color::Red);
    r1.setRotation(45);
    sf::RectangleShape r2;
    r2.setSize({60, 140});
    r2.setPosition({300, 120});
    r2.setFillColor(sf::Color::Green);


    window.create({1920, 1080}, "ImGui + SFML = <3");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window))
        return -1;

    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    while (window.isOpen())
    {
        sf::Event ev;
        while (window.pollEvent(ev))
        {
            ImGui::SFML::ProcessEvent(window, ev);

            if (ev.type == sf::Event::Closed) {
                window.close();
            }
        }
        ImGui::SFML::Update(window, deltaClock.restart());

        do_imgui(r1, r2);


        window.clear();
        window.draw(r1);
        window.draw(r2);
        draw_corners(r1);
        draw_corners(r2);
        ImGui::SFML::Render(window);
        window.display();
    }
}



bool vecInRct(ImVec2 v, ImVec2 min, ImVec2 max) {
    if (v.x >= min.x && v.x <= max.x &&
        v.y >= min.y && v.y <= max.y)
        return true;
    else
        return false;

}


void do_imgui(sf::RectangleShape &r1, sf::RectangleShape &r2)
{
    ImVec2 min1 = r1.getGlobalBounds().getPosition();
    ImVec2 max1 = r1.getGlobalBounds().getPosition() + r1.getGlobalBounds().getSize();
    ImVec2 min2 = r2.getGlobalBounds().getPosition();
    ImVec2 max2 = r2.getGlobalBounds().getPosition() + r2.getGlobalBounds().getSize();

    ImGuiIO &io = ImGui::GetIO();

    // ImGui::GetForegroundDrawList()->AddRectFilled(min1, max1, IM_COL32(255, 255, 255, 128));
    // ImGui::GetForegroundDrawList()->AddRectFilled(min2, max2, IM_COL32(255, 255, 255, 128));

    static bool dragging = false;
    static Rct *selected = nullptr;
    auto mousepos = ImGui::GetMousePos();

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (vecInRct(mousepos, min1, max1)) {
            selected = &rc1;
            dragging = true;
        } else if (vecInRct(mousepos, min2, max2)) {
            selected = &rc2;
            dragging = true;
        } else {
            selected = nullptr;
        }
    }

    if (dragging && selected && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        selected->x += io.MouseDelta.x;
        selected->y += io.MouseDelta.y;
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        dragging = false;
        selected = nullptr;
    }


    ImGui::ShowDemoWindow();
    ImGui::Begin("collisions");
    ImGui::Text("r1: (%g, %g, %g, %g, %g)", rc1.x, rc1.y, rc1.width, rc1.height, rc1.rotation);
    ImGui::Text("r2: (%g, %g, %g, %g, %g)", rc2.x, rc2.y, rc2.width, rc2.height, rc2.rotation);
    ImGui::Text("mouse: (%g, %g)", mousepos.x, mousepos.y);
    ImGui::Text("collides? : %s", colliding ? "yes" : "no");

    ImGui::SeparatorText("r1");
    ImGui::DragFloat("x##r1", &rc1.x);
    ImGui::DragFloat("y##r1", &rc1.y);
    ImGui::DragFloat("width##r1", &rc1.width);
    ImGui::DragFloat("height##r1", &rc1.height);
    ImGui::DragFloat("rotation##r1", &rc1.rotation);

    ImGui::SeparatorText("r2");
    ImGui::DragFloat("x##r2", &rc2.x);
    ImGui::DragFloat("y##r2", &rc2.y);
    ImGui::DragFloat("width##r2", &rc2.width);
    ImGui::DragFloat("height##r2", &rc2.height);
    ImGui::DragFloat("rotation##r2", &rc2.rotation);

    ImGui::End();

    rc1.toRectShape(r1);
    rc2.toRectShape(r2);
    colliding = collides(r1, r2);
}
