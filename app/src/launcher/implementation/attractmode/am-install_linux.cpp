// Unit Include
#include "am-install.h"

// Qx Includes
#include <qx/core/qx-regularexpression.h>
#include <qx/core/qx-system.h>

namespace Am
{
//===============================================================================================================
// INSTALL
//===============================================================================================================

QString Install::versionFromExecutable() const
{
    Qx::ExecuteResult res = Qx::execute(MAIN_EXE_PATH, {"--version"}, 1000);
    QRegularExpressionMatch sv = Qx::RegularExpression::SEMANTIC_VERSION.match(res.output);
    return sv.hasMatch() ? sv.captured() : QString();
}

}
