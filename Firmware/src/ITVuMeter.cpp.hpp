#pragma once
#include <FastLED.h>
#include "ITVuMeter.h"
#include "ITLedMap.h"
#include <Arduino.h>

#define BLUR_AMOUNT 2

ITVuMeter::ITVuMeter(const fl::XYMap &xymap) : fl::Fx2d(xymap), rate_{20}, lastGen_{0}, level_{0}, fadeRate_{15}, currentPalette_{PartyColors_p}
{
}


void ITVuMeter::draw(fl::Fx::DrawContext context)
{
    fadeToBlackBy(context.leds, fadeRate_);
    if((context.now - lastGen_) >= rate_){
        lastGen_ = context.now;
        //Orr use map_range_clamped
        int middle = getHeight()/2;
        int count = getHeight() - middle;
        int stopTop = middle + ((count + 1)*level_)/255;
        int stopBot = middle - ((count + 1)*level_)/255;
        for(int y=middle;y<stopTop;++y){
            CRGB color = ColorFromPalette(currentPalette_, map(y+1, middle, getHeight(), 0, 255));
            for(int x=0;x<getWidth();++x){
                fl::u16 index = getXYMap()(x,y);
                context.leds[index] = color;
            }
        }

        for(int y=middle;y>stopBot;--y){
            CRGB color = ColorFromPalette(currentPalette_, map(y+1, middle, getHeight(), 0, 255));
            for(int x=0;x<getWidth();++x){
                fl::u16 index = getXYMap()(x,y);
                context.leds[index] = color;
            }
        }
    }
}

fl::string ITVuMeter::fxName() const
{
    return "ITVuMeter";
}

void ITVuMeter::setLevel(fl::u8 level)
{
    if(level_ != level){
        level_ = level;
        lastGen_ = 0;
    }
}

void ITVuMeter::setRandomPalette()
{
    setPalette(random16());
}

void ITVuMeter::setPalette(int index)
{
    int palIndex = index % MAX_PALETTE; // Ensure the index wraps around
    switch (palIndex) {
    case 0:
        currentPalette_ = RainbowColors_p;
        // Serial.println("Rainbow");
        break;
    case 1:
        SetupPurpleAndGreenPalette();
        // Serial.println("PurpleAndGreen");
        break;
    case 2:
        SetupGrayAndWhiteStripedPalette();
        // Serial.println("GrayAndWhite");
        break;
    case 3:
        currentPalette_ = ForestColors_p;
        // Serial.println("ForestColors_p");
        break;
    case 4:
        currentPalette_ = CloudColors_p;
        // Serial.println("CloudColors_p");
        break;
    case 5:
        currentPalette_ = LavaColors_p;
        // Serial.println("LavaColors_p");
        break;
    case 6:
        currentPalette_ = OceanColors_p;
        // Serial.println("OceanColors_p");
        break;
    case 7:
        currentPalette_ = PartyColors_p;
        // Serial.println("PartyColors_p");
        break;
    }
}

void ITVuMeter::SetupGrayAndWhiteStripedPalette() {
    fill_solid(currentPalette_, 16, CRGB::Gray50);
    currentPalette_[0] = CRGB::White;
    currentPalette_[4] = CRGB::White;
    currentPalette_[8] = CRGB::White;
    currentPalette_[12] = CRGB::White;
}

void ITVuMeter::SetupPurpleAndGreenPalette() {
    CRGB purple = CHSV(HUE_PURPLE, 255, 255);
    CRGB green = CHSV(HUE_GREEN, 255, 255);
    CRGB yellow = CHSV(HUE_YELLOW, 255, 255);

    currentPalette_ = CRGBPalette16(
        green, green, yellow, yellow, purple, purple, yellow, yellow, green,
        green, yellow, yellow, purple, purple, yellow, yellow);
}