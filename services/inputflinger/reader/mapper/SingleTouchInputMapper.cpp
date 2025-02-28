/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SingleTouchInputMapper.h"

namespace android {

SingleTouchInputMapper::SingleTouchInputMapper(InputDeviceContext& deviceContext,
                                               const InputReaderConfiguration& readerConfig)
      : TouchInputMapper(deviceContext, readerConfig) {}

SingleTouchInputMapper::~SingleTouchInputMapper() {}

std::list<NotifyArgs> SingleTouchInputMapper::reset(nsecs_t when) {
    mSingleTouchMotionAccumulator.reset(getDeviceContext());

    return TouchInputMapper::reset(when);
}

std::list<NotifyArgs> SingleTouchInputMapper::process(const RawEvent& rawEvent) {
    std::list<NotifyArgs> out = TouchInputMapper::process(rawEvent);

    mSingleTouchMotionAccumulator.process(rawEvent);
    return out;
}

void SingleTouchInputMapper::syncTouch(nsecs_t when, RawState* outState) {
    if (mTouchButtonAccumulator.isToolActive()) {
        outState->rawPointerData.pointerCount = 1;
        outState->rawPointerData.idToIndex[0] = 0;

        bool isHovering = mTouchButtonAccumulator.getToolType() != ToolType::MOUSE &&
                (mTouchButtonAccumulator.isHovering() ||
                 (mRawPointerAxes.pressure &&
                  mSingleTouchMotionAccumulator.getAbsolutePressure() <= 0));
        outState->rawPointerData.markIdBit(0, isHovering);

        RawPointerData::Pointer& outPointer = outState->rawPointerData.pointers[0];
        outPointer.id = 0;
        outPointer.x = mSingleTouchMotionAccumulator.getAbsoluteX();
        outPointer.y = mSingleTouchMotionAccumulator.getAbsoluteY();
        outPointer.pressure = mSingleTouchMotionAccumulator.getAbsolutePressure();
        outPointer.touchMajor = 0;
        outPointer.touchMinor = 0;
        outPointer.toolMajor = mSingleTouchMotionAccumulator.getAbsoluteToolWidth();
        outPointer.toolMinor = mSingleTouchMotionAccumulator.getAbsoluteToolWidth();
        outPointer.orientation = 0;
        outPointer.distance = mSingleTouchMotionAccumulator.getAbsoluteDistance();
        outPointer.tiltX = mSingleTouchMotionAccumulator.getAbsoluteTiltX();
        outPointer.tiltY = mSingleTouchMotionAccumulator.getAbsoluteTiltY();
        outPointer.toolType = mTouchButtonAccumulator.getToolType();
        if (outPointer.toolType == ToolType::UNKNOWN) {
            outPointer.toolType = ToolType::FINGER;
        }
        outPointer.isHovering = isHovering;
    }
}

void SingleTouchInputMapper::configureRawPointerAxes() {
    TouchInputMapper::configureRawPointerAxes();

    // TODO(b/351870641): Investigate why we are sometime not getting valid axis infos for the x/y
    //   axes, even though those axes are required to be supported.
    if (const auto xInfo = getAbsoluteAxisInfo(ABS_X); xInfo.has_value()) {
        mRawPointerAxes.x = *xInfo;
    }
    if (const auto yInfo = getAbsoluteAxisInfo(ABS_Y); yInfo.has_value()) {
        mRawPointerAxes.y = *yInfo;
    }
    mRawPointerAxes.pressure = getAbsoluteAxisInfo(ABS_PRESSURE);
    mRawPointerAxes.toolMajor = getAbsoluteAxisInfo(ABS_TOOL_WIDTH);
    mRawPointerAxes.distance = getAbsoluteAxisInfo(ABS_DISTANCE);
    mRawPointerAxes.tiltX = getAbsoluteAxisInfo(ABS_TILT_X);
    mRawPointerAxes.tiltY = getAbsoluteAxisInfo(ABS_TILT_Y);
}

bool SingleTouchInputMapper::hasStylus() const {
    return mTouchButtonAccumulator.hasStylus();
}

} // namespace android
