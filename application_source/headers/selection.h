/**
 * @file selection.h
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include <objectshape.h>
#include <colors.h>

#include <cereal/types/utility.hpp>

#ifndef SELECTION
#define SELECTION

struct Selection
{
    template<class Archive>
    void serialize(Archive &archive)
    {
        archive(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(width), CEREAL_NVP(height), CEREAL_NVP(angle));
    }

    Selection() {  }

    Selection (int x, int y, int width, int height, int angle=0) :
        x(x),
        y(y),
        width(width),
        height(height),
        angle(angle)
    { }

    int x;
    int y;
    int width;
    int height;
    double angle; // todo: Should be double or int?
};

struct ObjectColor
{
    // CEREAL serialization
    template<class Archive>
    void serialize(Archive &archive)
    {
        archive(cereal::make_nvp("red", r), cereal::make_nvp("green", g), cereal::make_nvp("blue", b));
    }

    ObjectColor(): r(0), g(0), b(0) { } // default constructor
    ObjectColor(int r, int g, int b): r(r), g(g), b(b) { } // default constructor int r;
    int r;
    int g;
    int b;
};

struct Characteristics
{ //    Characteristics(): shape(ObjectShape::RECTANGLE), color(0,0,0), colorName("Black"), borderColor(0,0,0), borderColorName("Black"), borderThickness(3) { }

    // CEREAL serialization
    template<class Archive>
    void serialize(Archive &archive)
    {
        archive(CEREAL_NVP(shape), CEREAL_NVP(defocus), CEREAL_NVP(defocusSize), CEREAL_NVP(drawInside), CEREAL_NVP(color), CEREAL_NVP(colorID), CEREAL_NVP(drawBorder), CEREAL_NVP(borderColor), CEREAL_NVP(borderColorID), CEREAL_NVP(borderThickness));
    }

    // Default constructor
    Characteristics(): shape(ObjectShape::RECTANGLE), defocus(false), defocusSize(20),
            drawInside(true), color(ObjectColor(0, 0, 0)), colorID(Colors::BLACK),
            drawBorder(true), borderColor(ObjectColor(0,0,0)), borderColorID(Colors::BLACK),
            borderThickness(3) { }

    // Constructor with arguments
    Characteristics(unsigned int shape, bool defocus, unsigned int defocusSize, bool drawInside,
                        ObjectColor color, unsigned int colorID, bool drawBorder, ObjectColor borderColor,
                        unsigned int borderColorID, int borderThickness) :
            shape(shape), defocus(defocus), defocusSize(defocusSize), drawInside(drawInside), color(color),
            colorID(colorID), drawBorder(drawBorder), borderColor(borderColor), borderColorID(borderColorID),
            borderThickness(borderThickness) { }

    //ObjectShape shape;
    unsigned int shape;
    bool defocus;
    unsigned int defocusSize;
    bool drawInside;
    ObjectColor color;
    unsigned int colorID;
    bool drawBorder;
    ObjectColor borderColor;
    unsigned int borderColorID;
    int borderThickness;

};

#endif // SELECTION
