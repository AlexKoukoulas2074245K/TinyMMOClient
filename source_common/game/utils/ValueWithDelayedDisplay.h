///------------------------------------------------------------------------------------------------
///  ValueWithDelayedDisplay.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/12/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef ValueWithDelayedDisplay_h
#define ValueWithDelayedDisplay_h

///------------------------------------------------------------------------------------------------

#include <functional>

///------------------------------------------------------------------------------------------------

template<class T>
class ValueWithDelayedDisplay
{
public:
    ValueWithDelayedDisplay(const T initValue, const T initDisplayedValue = T(), std::function<void(const T&)> onNewValueSetCallback = nullptr)
        : mDisplayedValue(initDisplayedValue)
        , mOnNewValueSetCallback(onNewValueSetCallback)
    {
        SetValue(initValue);
    }
    
    ValueWithDelayedDisplay<T>& operator=(const ValueWithDelayedDisplay<T>& rhs)
    {
        mDisplayedValue = rhs.mDisplayedValue;
        
        if (rhs.mOnNewValueSetCallback)
        {
            mOnNewValueSetCallback = rhs.mOnNewValueSetCallback;
        }
        
        SetValue(rhs.mValue);
        
        return *this;
    }
    
    const T& GetValue() const { return mValue; }
    void SetValue(const T& value)
    {
        mValue = value;
        if (mOnNewValueSetCallback)
        {
            mOnNewValueSetCallback(mValue);
        }
    }
    
    const T& GetDisplayedValue() const { return mDisplayedValue; }
    void SetDisplayedValue(const T& displayedValue) { mDisplayedValue = displayedValue; }
    
protected:
    T mValue;
    T mDisplayedValue;
    std::function<void(const T&)> mOnNewValueSetCallback;
};

///------------------------------------------------------------------------------------------------

#endif /* ValueWithDelayedDisplay_h */
