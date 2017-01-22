#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <set>
#include <string.h>

#include "gumbo.h"

static void search_for_links(GumboNode* node, std::set<std::string> &urls);
void extract_urls(char *filename, std::set<std::string> &urls);