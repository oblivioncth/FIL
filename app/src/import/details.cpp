// Unit Includes
#include "details.h"

namespace Import
{

//===============================================================================================================
// Details
//===============================================================================================================

//-Class Variables--------------------------------------------------------------------------------------------
//Private:
constinit std::optional<Details> Details::mCurrent = std::nullopt;

//-Class Functions--------------------------------------------------------------------------------------------
//Public:
Details Details::current() { Q_ASSERT(mCurrent); return mCurrent.value(); }

//Private:
void Details::setCurrent(const Details& details) { Q_ASSERT(!mCurrent); mCurrent = details; }
void Details::clearCurrent() { mCurrent = std::nullopt; }

}
