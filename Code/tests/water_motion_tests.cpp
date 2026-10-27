#include "water_motion.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
}
int main()
{
    try {
        using namespace watermotion;
        auto p = graphics::defaults(0);
        History h;
        Input input;
        h.append(input, p);
        for (int i = 1; i <= 100; ++i) {
            input.time = i * 0.1;
            input.center = {i * 0.01, 0, 0};
            input.velocity = {0.1, 0, 0};
            h.append(input, p);
        }
        require(h.points().size() == 5, "spatial samples must not depend on frame count");
        require(std::abs(h.points().back().distance - 1.0) < 1e-10, "sample length");
        const auto first = h.points();
        h.clear();
        input = {};
        h.append(input, p);
        input.time = 10;
        input.center = {1, 0, 0};
        input.velocity = {0.1, 0, 0};
        h.append(input, p);
        require(h.points().size() == first.size(), "frame skip changes sample count");
        for (std::size_t i = 0; i < first.size(); ++i) {
            require(std::abs(h.points()[i].time - first[i].time) < 1e-9, "frame skip changes sample time");
            require(forcedisplay::norm(forcedisplay::sub(h.points()[i].center, first[i].center)) < 1e-9,
                "frame skip changes sample position");
        }
        for (int i = 0; i < 10; ++i) {
            input.time += 0.1;
            input.center[2] += 0.1;
            input.velocity = {};
            h.append(input, p);
        }
        require(h.points().size() == 5, "rotation/heave around stationary COM creates distance");
        input.time += 0.1;
        input.reference = {1, 0, 0};
        input.center = {100, 0, 0};
        h.append(input, p);
        require(!h.points().back().connected && !h.diagnostic().isEmpty(), "COM reference change joins path");
        p.trackBudget = 4;
        input.time += 1;
        input.center[0] += 10000;
        h.append(input, p);
        require(h.points().size() <= 4, "large step exceeds budget");
        p.trackAge = 1;
        input.time += 2;
        h.append(input, p);
        require(h.points().empty(), "stationary samples never age out");
        input.time = 0;
        h.append(input, p);
        require(h.points().size() == 1, "rewind not reset");
        QVector<Vec3> contour{{-2,-1,0},{-2,1,0},{2,-1,0},{2,1,0}};
        const auto forward = contactFromContour(contour, {1,0,0});
        const auto reverse = contactFromContour(contour, {-1,0,0});
        const auto drift = contactFromContour(contour, {0,1,0});
        require(forward && reverse && drift, "valid contact rejected");
        require(forward->leading[0] == 2 && reverse->leading[0] == -2
            && drift->leading[1] == 1, "contact must follow actual movement");
        require(!contactFromContour({}, {1,0,0}) && !contactFromContour(contour, {}),
            "no contact/stationary water creates wake");
        p = graphics::defaults(0);
        h.clear();
        input = {};
        h.append(input, p);
        input.time = 1;
        input.center = {5000,0,0};
        input.velocity = {5000,0,0};
        h.append(input, p);
        require(h.points().size() == 4096, "exact default 4096-point threshold");
        require(h.points().front().distance == 5000 - 4095 * 0.25
            && h.points().back().distance == 5000, "budget must retain latest spatial samples");
        input.time = 31;
        input.velocity = {};
        h.append(input, p);
        require(h.points().size() == 1, "30-second age boundary");
        input.time = 31.000001;
        h.append(input, p);
        require(h.points().empty(), "age beyond exact default threshold");
        input.time = std::numeric_limits<double>::quiet_NaN();
        bool rejected = false;
        try { h.append(input, p); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "non-finite history accepted");
        std::cout << "Water history: spatial interpolation, rotation/heave, reference breaks, age/budget, rewind and contact direction passed.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
