
#include "pattern_matcher/RepeatCount.h"

RepeatCount::RepeatCount(size_t aMin, size_t aMax)
	: myMin(aMin)
	, myMax(aMax)
{

}

RepeatCount::RepeatCount(size_t aFixed)
	: myMin(aFixed)
	, myMax(aFixed)
{

}