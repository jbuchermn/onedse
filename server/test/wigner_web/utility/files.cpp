#include <gtest/gtest.h>
#include <iostream>
#include <fstream>

#include "wigner_web/utility/files.h"

TEST(files, index_html){
    wigner_web::utility::files::index_html();
}

TEST(files, session_lua){
    wigner_web::utility::files::session_lua(0);
}
