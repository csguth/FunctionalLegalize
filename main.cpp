#include <iostream>
#include <vector>
#include <random>
#include <cstdint>
#include <array>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <fstream>
#include <map>
#include <functional>
#include <utility>

template <class T>
struct Point
{
    using value_type = T;
    const value_type x;
    const value_type y;

    friend void swap(Point<T>& a, Point<T>& b)
    {
        std::swap(a.x, b.x);
        std::swap(a.y, b.y);
    }

    Point<value_type> operator+(const Point<value_type>& o) const
    {
        return {x + o.x, y + o.y};
    }

    Point<value_type> operator/(value_type o) const
    {
        return {x/o, y/o};
    }

    Point<value_type> operator/(const Point<value_type>& o) const
    {
        return {x/o.x, y/o.y};
    }

    Point<value_type> operator*(const Point<value_type>& o) const
    {
        return {x*o.x, y*o.y};
    }

};

struct Rectangle
{
    using point_type = Point<float>;
    const point_type position;
    const point_type size;

    point_type lowerCorner() const
    {
        return position;
    }

    point_type upperCorner() const
    {
        return position + size;
    }

    point_type center() const
    {
        return lowerCorner() + (size / 2.0);
    }
};

namespace std
{
template <class T>
Point<T> floor(const Point<T>& p)
{
    return {std::floor(p.x), std::floor(p.y)};
}

}

std::default_random_engine engine;
template <class T>
Rectangle generate(const Rectangle& boundaries, T& library)
{
    auto avg = boundaries.center();
    auto std = boundaries.size/5.0;
    std::normal_distribution<Rectangle::point_type::value_type> distX{avg.x, std.x}, distY{avg.y, std.y};
    std::uniform_int_distribution<typename T::size_type> distLibrary(0, library.size()-1);
    auto index = distLibrary(engine);
    return {library[index].position + Rectangle::point_type{distX(engine), distY(engine)}, library.at(index).size};
}

template <typename T>
void writeSvg(std::ostream& out, const Rectangle& boundaries, T rectangles)
{
    out << "<svg version=\"1.1\" baseProfile=\"full\" width=\"" << boundaries.size.x << "\" height=\"" << boundaries.size.y << "\">" << std::endl;

    enum class Color
    {
        White,
        Black,
        Red,
        Green,
        Blue
    };

    auto rectToSvg = [](const Rectangle& rect, Color fill = Color::White) -> std::string
    {
        static const std::map<Color, std::string> colorNameMaping = {
            {Color::White, "white"},
            {Color::Black, "black"},
            {Color::Red, "red"},
            {Color::Green, "green"},
            {Color::Blue, "blue"}
        };

        std::stringstream ss;
        ss << "    <rect ";
        ss << "x=\"" << rect.position.x << "\" ";
        ss << "y=\"" << rect.position.y << "\" ";
        ss << "width=\"" << rect.size.x << "\" ";
        ss << "height=\"" << rect.size.y << "\" ";
        ss << "fill=\"" << colorNameMaping.at(fill) << "\"";
        ss << "/>";
        return ss.str();
    };

    out << rectToSvg(boundaries) << std::endl;
    std::transform(std::begin(rectangles), std::end(rectangles), std::ostream_iterator<std::string>(out, "\n"), std::bind(rectToSvg, std::placeholders::_1, Color::Red));
    out << "</svg>";
    out << std::endl;
}


template <typename T>
void writeSvg(const char out[], const Rectangle& boundaries, T rectangles)
{
    std::ofstream output(out, std::ofstream::out);
    writeSvg(output, boundaries, rectangles);
}

std::vector<Rectangle> mergeSort(std::vector<Rectangle>::const_iterator begin, std::vector<Rectangle>::const_iterator end)
{
    if(std::distance(begin, end) == 1)
        return {*begin};
    auto middle = begin + (std::distance(begin, end)/2);
    auto first = mergeSort(begin, middle);
    auto second = mergeSort(middle, end);
    std::vector<Rectangle> merged;
    merged.reserve(std::distance(begin, end));
    std::merge(first.begin(), first.end(), second.begin(), second.end(), std::back_inserter(merged), [](const Rectangle&a, const Rectangle&b) -> bool {
        return a.center().x < b.center().x;
    });
    return std::move(merged);
}

std::vector<Rectangle> snapToGrid(std::vector<Rectangle> in, const Point<float>& grid)
{
    std::vector<Rectangle> out;
    out.reserve(in.size());
    std::transform(in.begin(), in.end(), std::back_inserter(out), [&grid](const Rectangle& rect) -> Rectangle {
        auto snappedPosition = std::floor(rect.position/grid)*grid;
        return {snappedPosition, rect.size};
    });
    return std::move(out);
}

std::vector<Rectangle> makePartition(std::vector<Rectangle> in, std::function<bool(const Rectangle&)> pred)
{
    std::vector<Rectangle> out;
    out.reserve(in.size());
    std::copy_if(in.begin(), in.end(), std::back_inserter(out), pred);
    return std::move(out);
}

std::vector<Rectangle> legalize(std::vector<Rectangle> in)
{
    std::vector<Rectangle> out;
    out.reserve(in.size());
    out.push_back(in.front());
    for(uint32_t i = 1; i < in.size(); ++i)
    {
        const Rectangle& rect = in.at(i);
        const Rectangle& prev = out.at(i-1);
        Rectangle output{{std::max(rect.position.x, (prev.position+prev.size).x), rect.position.y}, rect.size};
        out.push_back(output);
    }
    return out;
}

std::vector<Rectangle> sortByX(std::vector<Rectangle> in)
{
    return mergeSort(in.cbegin(), in.cend());
}

std::vector<Rectangle> join(std::vector<std::vector<Rectangle>> in)
{
    const uint32_t total = std::accumulate(in.begin(), in.end(), 0, [](const uint32_t lhs, auto& vec)-> uint32_t{
        return lhs + vec.size();
    });
    std::vector<Rectangle> out;
    out.reserve(total);
    for(auto& v: in)
    {
        for(auto& el: v)
        {
            out.push_back(el);
        }
    }
    return std::move(out);
}

std::vector<std::vector<Rectangle>> makeLegalizedPartitions(std::vector<Rectangle> in, const Rectangle& boundaries, const Point<float>& grid)
{
    std::vector<std::vector<Rectangle>> partitions((boundaries.size/grid).y);
    for(uint32_t i = 0; i < partitions.size(); ++i)
    {
        partitions[i] = legalize(makePartition(in, [&grid, i](const Rectangle& rect){
            return rect.position.y==(grid.y*i);
        }));
    }
    return std::move(partitions);
}

int main(int argc, char *argv[])
{

    static constexpr uint32_t N{100000};
    const Rectangle boundaries{ {0.0, 0.0}, {300000.0, 300000.0} };
    static constexpr Point<float> Grid{1.0, 200.0};
    static constexpr std::array<Rectangle, 4> library = {{
        {{0.0, 0.0}, {20.0*Grid.x, Grid.y}},
        {{0.0, 0.0}, {40.0*Grid.x, Grid.y}},
        {{0.0, 0.0}, {160.0*Grid.x, Grid.y}},
        {{0.0, 0.0}, {320.0*Grid.x, Grid.y}}
    }};

    std::vector<Rectangle> cells;
    cells.reserve(N);
    for(uint32_t i = 0; i < N; ++i)
    {
        cells.push_back(generate(boundaries, library));
    }

    writeSvg("0-input.svg", boundaries, cells);
    writeSvg("1-legalized.svg", boundaries, join(makeLegalizedPartitions(sortByX(snapToGrid(cells, Grid)), boundaries, Grid)));

    return 0;
}
