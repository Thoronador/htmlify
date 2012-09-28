/*
 -------------------------------------------------------------------------------
    This file is part of htmlify.
    Copyright (C) 2012  Thoronador

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 -------------------------------------------------------------------------------
*/

#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <cstring>
#include "pmdb/libthoro/common/FileFunctions.h"
#include "pmdb/libthoro/common/StringConversion.h"
#include "handleSpecialChars.h"
#include "pmdb/MsgTemplate.h"
#include "pmdb/bbcode/BBCode.h"
#include "pmdb/bbcode/BBCodeParser.h"
#include "TrimmingBBCodes.h"

//return codes
const int rcInvalidParameter = 1;
const int rcFileError        = 2;
const int rcConversionFail   = 3;

void showGPLNotice()
{
  std::cout << "htmlify\n"
            << "  Copyright (C) 2012 Thoronador\n"
            << "\n"
            << "  This programme is free software: you can redistribute it and/or\n"
            << "  modify it under the terms of the GNU General Public License as published\n"
            << "  by the Free Software Foundation, either version 3 of the License, or\n"
            << "  (at your option) any later version.\n"
            << "\n"
            << "  This programme is distributed in the hope that they will be useful,\n"
            << "  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
            << "  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
            << "  GNU General Public License for more details.\n"
            << "\n"
            << "  You should have received a copy of the GNU General Public License\n"
            << "  along with this programme.  If not, see <http://www.gnu.org/licenses/>.\n"
            << "\n";
}

void showVersion()
{
  showGPLNotice();
  std::cout << "htmlify, version 0.03, 2012-09-28\n";
}

void showHelp(const std::string& name)
{
  std::cout << "\n"<<name<<" [--html|--xhtml] FILENAME\n"
            << "options:\n"
            << "  --help           - displays this help message and quits\n"
            << "  -?               - same as --help\n"
            << "  --version        - displays the version of the programme and quits\n"
            << "  -v               - same as --version\n"
            << "  FILENAME         - adds file FILENAME to the list of text files that should\n"
            << "                     be processed. The file must exist, or the programme will\n"
            << "                     abort. Can be repeated multiple times for different\n"
            << "                     files.\n"
            << "  --html           - uses HTML (4.01) syntax for generated files. Enabled by\n"
            << "                     default.\n"
            << "  --xhtml          - uses XHTML syntax for generated files. Mutually exclusive\n"
            << "                     with --html.\n"
            << "  --trim=PREFIX    - removes PREFIX from link URLs, if they start with PREFIX.\n"
            << "  --utf8           - content of input files is encoded in UTF-8 and will be\n"
            << "                     converted to ISO-8859-15 before processing.\n";
}

int main(int argc, char **argv)
{
  std::set<std::string> pathTexts;
  bool doXHTML = false;
  bool htmlModeSpecified = false;
  std::string trimmablePrefix = "";
  bool isUTF8 = false;

  if ((argc>1) and (argv!=NULL))
  {
    int i=1;
    while (i<argc)
    {
      if (argv[i]!=NULL)
      {
        const std::string param = std::string(argv[i]);
        //help parameter
        if ((param=="--help") or (param=="-?") or (param=="/?"))
        {
          showHelp(argv[0]);
          return 0;
        }//param == help
        //version information requested?
        else if ((param=="--version") or (param=="-v"))
        {
          showVersion();
          return 0;
        }//param == version
        else if ((param=="--html") or (param=="--html4"))
        {
          if (htmlModeSpecified)
          {
            std::cout << "Parameter "<<param<<" must not occur more than once "
                      << "and is mutually exclusive with --xhtml!\n";
            return rcInvalidParameter;
          }
          doXHTML = false;
          htmlModeSpecified = true;
        }//param == html
        else if ((param=="--xhtml") or (param=="--XHTML"))
        {
          if (htmlModeSpecified)
          {
            std::cout << "Parameter "<<param<<" must not occur more than once "
                      << "and is mutually exclusive with --html!\n";
            return rcInvalidParameter;
          }
          doXHTML = true;
          htmlModeSpecified = true;
        }//param == xhtml
        else if ((param=="-t") or (param=="--trim"))
        {
          if ((i+1<argc) and (argv[i+1]!=NULL))
          {
            if (!trimmablePrefix.empty())
            {
              std::cout << "Parameter "<<param<<" must not occur more than once!\n";
              return rcInvalidParameter;
            }
            trimmablePrefix = std::string(argv[i+1]);
            ++i; //skip next parameter, because it's used as prefix already
            std::cout << "Trimmable prefix was set to \""<<trimmablePrefix<<"\".\n";
          }
          else
          {
            std::cout << "Error: You have to specify a string after \""
                      << param <<"\".\n";
            return rcInvalidParameter;
          }
        }//param == trim
        else if ((param.substr(0,7)=="--trim=") and (param.length()>7))
        {
          if (!trimmablePrefix.empty())
          {
            std::cout << "Parameter --trim must not occur more than once!\n";
            return rcInvalidParameter;
          }
          trimmablePrefix = param.substr(7);
          std::cout << "Trimmable prefix was set to \""<<trimmablePrefix<<"\".\n";
        }//param == trim (single parameter version)
        else if ((param=="--utf8") or (param=="--UTF-8"))
        {
          if (isUTF8)
          {
            std::cout << "Parameter "<<param<<" must not occur more than once!\n";
            return rcInvalidParameter;
          }
          isUTF8 = true;
        }//param == utf8
        else if (FileExists(param))
        {
          if (pathTexts.find(param)!=pathTexts.end())
          {
            std::cout << "File \""<<param<<"\" was specified more than once!\n";
            return rcInvalidParameter;
          }
          pathTexts.insert(param);
        }
        else
        {
          //unknown or wrong parameter
          std::cout << "Invalid parameter given: \""<<param<<"\".\n"
                    << "Use --help to get a list of valid parameters.\n";
          return rcInvalidParameter;
        }
      }//parameter exists
      else
      {
        std::cout << "Parameter at index "<<i<<" is NULL.\n";
        return rcInvalidParameter;
      }
      ++i;//on to next parameter
    }//while
  }//if arguments present

  //no files to load?
  if (pathTexts.empty())
  {
    std::cout << "You have to specify certain parameters for this programme to run properly.\n"
              << "Use --help to get a list of valid parameters.\n";
    return rcInvalidParameter;
  }

  //prepare BB codes
  // [b], [u], [i], [s] codes
  SimpleBBCode b("b");
  SimpleBBCode u("u");
  SimpleBBCode i("i");
  CustomizedSimpleBBCode s("s",
                           "<span style=\"text-decoration:line-through;\">",
                           "</span>");
  //[sup] and [sub] tags
  SimpleBBCode sup("sup");
  SimpleBBCode sub("sub");
  //indent tags
  CustomizedSimpleBBCode indent("indent", "<blockquote>", "</blockquote>");
  //alignment stuff
  SimpleBBCode center("center");
  CustomizedSimpleBBCode left("left", "<div align=\"left\">", "</div>");
  CustomizedSimpleBBCode right("right", "<div align=\"right\">", "</div>");
  //image tags
  CustomizedSimpleBBCode img_simple("img", "<img src=\"",
                                    doXHTML ? "\" alt=\"\" />" : "\" alt=\"\">");
  //simple url tag
  MsgTemplate tpl;
  tpl.loadFromString("<a href=\"{..inner..}\" target=\"_blank\">{..inner..}</a>");
  SimpleTplAmpTransformBBCode url_simple("url", tpl, "inner");
  SimpleTrimBBCode url_simple_trim("url", tpl, "inner", trimmablePrefix);
  //advanced url tag
  tpl.loadFromString("<a href=\"{..attribute..}\" target=\"_blank\">{..inner..}</a>");
  AdvancedTplAmpTransformBBCode url_advanced("url", tpl, "inner", "attribute");
  AdvancedTrimBBCode url_advanced_trim("url", tpl, "inner", "attribute", trimmablePrefix);
  //color tags
  tpl.loadFromString("<font color=\"{..attr..}\">{..inner..}</font>");
  AdvancedTemplateBBCode color("color", tpl, "inner", "attr");
  //size tags
  tpl.loadFromString("<font size=\"{..attr..}\">{..inner..}</font>");
  AdvancedTemplateBBCode size("size", tpl, "inner", "attr");

  //add it to the parser
  BBCodeParser parser;
  parser.addCode(&b);
  parser.addCode(&u);
  parser.addCode(&i);
  parser.addCode(&s);
  parser.addCode(&sup);
  parser.addCode(&sub);
  parser.addCode(&indent);
  parser.addCode(&center);
  parser.addCode(&left);
  parser.addCode(&right);
  parser.addCode(&img_simple);
  if (trimmablePrefix.empty())
  {
    parser.addCode(&url_simple);
    parser.addCode(&url_advanced);
  }
  else
  {
    parser.addCode(&url_simple_trim);
    parser.addCode(&url_advanced_trim);
  }
  parser.addCode(&color);
  parser.addCode(&size);

  std::set<std::string>::const_iterator iter = pathTexts.begin();
  while (iter!=pathTexts.end())
  {
    //read file contents
    std::ifstream input;
    input.open(iter->c_str(), std::ios_base::in | std::ios_base::binary);
    if (!input)
    {
      std::cout << "Error: could not open file \""<<*iter<<"\"!\n";
      return rcFileError;
    }
    input.seekg(0, std::ios_base::end);
    const std::streamsize len = input.tellg();
    input.seekg(0, std::ios_base::beg);
    if (!input.good())
    {
      #ifdef DEBUG
      std::cout << "Error while reading file content: seek operation failed!\n";
      #endif
      input.close();
      return rcFileError;
    }
    if (len>1024*1024)
    {
      #ifdef DEBUG
      std::cout << "Error while reading file content: unexpectedly large file size!\n";
      #endif
      input.close();
      return rcFileError;
    }
    //allocate buffer - all file content should fit into it
    char * buffer = new char[len+1];
    memset(buffer, '\0', len+1); //zerofill buffer
    input.read(buffer, len);
    if (!input.good())
    {
      delete[] buffer;
      buffer = NULL;
      input.close();
      std::cout << "Error while reading file content of \""<<*iter<<"\"!\n";
      return rcFileError;
    }
    input.close();

    std::string content(buffer);
    delete[] buffer;
    buffer = NULL;

    if (isUTF8)
    {
      //convert content to iso-8859-15
      std::string iso_content;
      if (!Thoro::utf8_to_iso8859_15(content, iso_content))
      {
        std::cout << "Error: Conversion from UTF-8 failed!\n";
        return rcConversionFail;
      }
      content = iso_content;
    }//if UTF-8

    handleSpecialChars(content);
    content = parser.parse(content, "", doXHTML, false);

    //save content
    std::ofstream output;
    output.open((*iter + "_htmlified").c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    if (!output)
    {
      std::cout << "Could not open output file "<<*iter<<"_htmlified for writing!\n";
      return rcFileError;
    }
    output.write(content.c_str(), content.length());
    if (!output.good())
    {
      std::cout << "Error while writing to file \""<<*iter<<"_htmlified\"!\n";
      output.close();
      return rcFileError;
    }
    output.close();
    std::cout << "Processed file "<<*iter<<"\n";
    ++iter;
  }//while
  return 0;
}
