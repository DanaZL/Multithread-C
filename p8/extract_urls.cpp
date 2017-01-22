#include "extract_urls.h"
  

int check_href (const char *href) {
  // std::cout << href << std::endl;
  if (!(strncmp("http://", href, 7)) || !(strncmp("mailto:", href, 7))
     || ((strlen(href) == 1) && href[0] == '/') || !strcmp(href, "")) {
    // std::cout << href << "\thref" << std::endl;
    return 0;
  }
  return 1;
}

static void search_for_links (GumboNode* node, std::set<std::string> &urls) {
  if (node->type != GUMBO_NODE_ELEMENT) {
    return;
  }

  GumboAttribute* href;
  if (node->v.element.tag == GUMBO_TAG_A &&
      (href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
      if (check_href(href->value)) {
        // std::cout << "PREV URLS\t" << href->value << std::endl;
        urls.insert(std::string(href->value));
      }
  }

  GumboVector* children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    search_for_links(static_cast<GumboNode*>(children->data[i]), urls);
  }
}

void extract_urls(char *filename, std::set<std::string> &urls)
{
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (!in) {
    std::cout << "File " << filename << " not found!\n";
    exit(EXIT_FAILURE);
  }

  std::string contents;
  in.seekg(0, std::ios::end);
  contents.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents[0], contents.size());
  in.close();

  GumboOutput* output = gumbo_parse(contents.c_str());

  search_for_links(output->root, urls);

  gumbo_destroy_output(&kGumboDefaultOptions, output);
}