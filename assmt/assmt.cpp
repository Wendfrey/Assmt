#include <iostream>
#include <fstream>
#include <tinyxml2.h>
#include <string>
#include <map>
#include <filesystem>
#include <vector>
#include <fstream>


using namespace tinyxml2;
namespace fs = std::filesystem;
using std::cout;
using std::cin;
using std::string;
using std::vector;

const char* strings_literal{ "strings" };
const char* id_attribute_name{ "id" };
const int BUFFER_SIZE{ 1024 };
const int ELEMENT_SIZE{ sizeof(char)};

class MergeData {
public:
    bool deleteSourceOnly{ false };
    bool keep_target_attrs{ false };
    bool output_is_target{ false };
    string sourceFile{};
    string targetFile{};
    string outputFile{};
};

class XmlData {
public:
    std::map<string, string> attributes;
    string idValue{};
};

enum AssmtErrorEnum {
    UNKNOWN_ERROR = 0,
    DIR_NOT_FOUND = 1,
    UNKNOWN_ARGUMENT = 2,
    TOO_MANY_PARAMS = 3,
    UNKNOWN_COMMAND = 4,
    DIR_NOT_VALID = 5,
    ERROR_W_FILE = 6,
    DIR_ISNOT_FOLDER = 7,
    INVALID_FILE_EXT = 8,
    ERROR_R_XML = 9,
    TOO_FEW_PARAMS = 10,
    INPUT_ERROR = 11,
    FILE_DOESNT_EXIST = 12,
    INVALID_FILE = 13,
    EMPTY_CONF_FILE = 14
};

template<typename Map>
bool map_compare(Map const& lhs, Map& rhs) {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](auto a, auto b) { return (a.first == b.first && a.second == b.second); });
}

std::map<string, string> map_merge(std::map<string, string>  source, std::map<string, string>  const target) {
    std::map<string, string> ret_value;
    std::map<string, string>::iterator other;
    for (auto it{ target.begin() }; it != target.end(); it++) {
        if ((other = source.find(it->first)) == source.end()) {
            ret_value.insert(std::pair<string, string>(it->first, it->second));
        }
        else {
            ret_value.insert(std::pair<string, string>(other->first, other->second));
            source.erase(other);
        }
    }

    for (auto it{ source.begin() }; it != source.end(); it++) {
        ret_value.insert(std::pair<string, string>(it->first, it->second));
    }

    return ret_value;
}

int get_argument_from_cin(vector<string>& arguments) {
    string linedata{};
    std::getline(cin, linedata);
    if (!cin.good()) return 1;
    bool search_word_first_c{ true };
    int first_word_char{ 0 };
    bool comillas_found{ false };
    char act_char{};
    for (unsigned int index{}; index < linedata.size(); index++) {
        act_char = linedata.at(index);
        if (search_word_first_c) {
            if (!isspace(act_char)) {
                search_word_first_c = false;
                first_word_char = index;
                if (act_char == '"') {
                    comillas_found = true;
                }
            }
        }
        else {
            if (isspace(act_char)) {
                if (!comillas_found) {
                    arguments.push_back(linedata.substr(first_word_char, index - first_word_char));
                    search_word_first_c = true;
                }
            }
            else if (comillas_found && act_char == '"') {
                arguments.push_back(linedata.substr(first_word_char + 1, index - first_word_char - 1));
                comillas_found = false;
                search_word_first_c = true;
            }
        }
    }
    if (comillas_found) return 2;
    if (!search_word_first_c) arguments.push_back(linedata.substr(first_word_char, string::npos));
    if (arguments.size() <= 0) return 3;
    return 0;
}

int loop_get_argument_from_cin(vector<string>& arguments) {
    int error{};
    do {
        error = get_argument_from_cin(arguments);
        if (error != 0 && error != 3)
            return error;
    } while (arguments.size() <= 0);
    return error;
}

void show_header() {
    cout << "            _____ _____ __  __ _______ " << std::endl;
    cout << "     /\\    / ____/ ____|  \\/  |__   __|" << std::endl;
    cout << "    /  \\  | (___| (___ | \\  / |  | |   " << std::endl;
    cout << "   / /\\ \\  \\___ \\\\___ \\| |\\/| |  | |   " << std::endl;
    cout << "  / ____ \\ ____) |___) | |  | |  | |   " << std::endl;
    cout << " /_/    \\_\\_____/_____/|_|  |_|  |_|   " << std::endl;
    cout << "* * * * * * * * * * * * * * * * * * * *" << std::endl;
    cout << "*      Android Studio Merge Tool      *" << std::endl;
    cout << "* * * * * * * * * * * * * * * * * * * *" << std::endl;

    cout << "* * * * Created by Antonio Cano * * * *" << std::endl;
    cout << " This software uses tinyxml2 " << std::endl;
    cout << "======================================" << std::endl;
}

void show_error(string context, AssmtErrorEnum error_type) {
    string error_message{};
    switch (error_type) {
    case DIR_NOT_FOUND: error_message = "Directory argument not found."; break;
    case UNKNOWN_ARGUMENT: error_message = "Unknown argument."; break;
    case TOO_MANY_PARAMS: error_message = "Too many parameters!"; break;
    case UNKNOWN_COMMAND: error_message = "Unknown command"; break;
    case DIR_NOT_VALID: error_message = "The directories are not valid"; break;
    case ERROR_W_FILE: error_message = "An unexpected error ocurred when writing in the file."; break;
    case DIR_ISNOT_FOLDER: error_message = "The route is not a valid directory (it must be a folder!)."; break;
    case INVALID_FILE_EXT: error_message = "Invalid file extension"; break;
    case ERROR_R_XML: error_message = "Unexpected error reading xml file"; break;
    case TOO_FEW_PARAMS: error_message = "Too few parameters!"; break;
    case INPUT_ERROR: error_message = "Couldn't read correctly the input"; break;
    case FILE_DOESNT_EXIST: error_message = "File doesn't exists"; break;
    case INVALID_FILE: error_message = "Invalid file"; break;
    case EMPTY_CONF_FILE: error_message = "Empty conf file"; break;
    default: error_message = "unknown error."; break;
    }

    std::cerr << "*** Error *** - " << error_message << ": " << context<< std::endl;
   
}

int loadXmlToMap(FILE** xmlFile, std::map<string, XmlData>& map) {
    int ret_value{};
    XMLError xmlError;
    XMLDocument doc;
    if ((xmlError = doc.LoadFile(*xmlFile)) == 0) {
        XMLElement* rootElement{ doc.RootElement() };
        if (strcmp(strings_literal, rootElement->Name()) != 0) {
            ret_value = 2;
            show_error("Invalid root name", ERROR_R_XML);
        }
        else {
            XMLNode* stringElement{ rootElement->FirstChild() };
            while (stringElement != nullptr) {

                const XMLAttribute* idAttr = stringElement->ToElement()->FindAttribute(id_attribute_name);
                if (idAttr == nullptr) {
                    show_error("Attribute id not found", ERROR_R_XML);
                    ret_value = 3;
                    break;
                }
                string key{ idAttr->Value() };
                string value{ stringElement->ToElement()->GetText() };
                XmlData xmlData;
                xmlData.idValue = value;
                for (const XMLAttribute* attr = stringElement->ToElement()->FirstAttribute(); attr != 0; attr = attr->Next()) {
                    if (strcmp(id_attribute_name, attr->Name()) != 0) {
                        xmlData.attributes.insert(std::pair<string, string>(attr->Name(), attr->Value()));
                    }
                }

                map.insert(std::pair<string, XmlData>(key, xmlData));
                stringElement = stringElement->NextSibling();
            }
        }
    }
    else {
        show_error("Unable to load xml", ERROR_R_XML);
        ret_value = 1;
    }
    return ret_value;
}

std::map<string, XmlData> mergeXmlData(std::map<string, XmlData> source_data, std::map<string, XmlData> target_data, MergeData mData) {
    std::map<string, XmlData> ret_data;

    cout << "=========== CHANGES ================" << std::endl;

    std::map<string, XmlData> finalList;
    auto sourceIt = source_data.begin();
    auto targetIt = target_data.begin();
    //SEARCH IN BOTH SIDES THE KEY ID AND COMPARE
    while (sourceIt != source_data.end() && targetIt != target_data.end()) {
        if (sourceIt->first == targetIt->first) {
            XmlData newData;
            if (sourceIt->second.idValue == targetIt->second.idValue) {
                if (map_compare(sourceIt->second.attributes, targetIt->second.attributes)) {
                    newData = sourceIt->second;
                    cout << "\tNo changes id " << sourceIt->first << std::endl;
                }
                else if (mData.keep_target_attrs) {
                    newData.idValue = sourceIt->first;
                    newData.attributes = map_merge(sourceIt->second.attributes, targetIt->second.attributes);
                    cout << "\tUpdated attributes of id " << sourceIt->first << std::endl;
                }
                else {
                    newData = sourceIt->second;
                    cout << "\tUpdated attributes of id " << sourceIt->first << std::endl;
                }
            }
            else {
                if (map_compare(sourceIt->second.attributes, targetIt->second.attributes)) {
                    newData = sourceIt->second;
                    cout << "\tUpdate value of id " << sourceIt->first << std::endl;
                }
                else if (mData.keep_target_attrs) {
                    newData.idValue = sourceIt->first;
                    newData.attributes = map_merge(sourceIt->second.attributes, targetIt->second.attributes);
                    cout << "\tUpdate value and attributes of id " << sourceIt->first << std::endl;
                }
                else {
                    newData = sourceIt->second;
                    cout << "\tUpdate value and attributes of id " << sourceIt->first << std::endl;
                }
            }
            ret_data.insert(std::pair<string, XmlData>(sourceIt->first, newData));
            sourceIt++;
            targetIt++;
        }
        else if (sourceIt->first < targetIt->first) {
            cout << "\tAdded id " << sourceIt->first << std::endl;
            ret_data.insert(std::pair<string, XmlData>(sourceIt->first, sourceIt->second));
            sourceIt++;
        }
        else {
            cout << "\tId " << targetIt->first << " not found in source" << std::endl;
            ret_data.insert(std::pair<string, XmlData>(targetIt->first, targetIt->second));
            targetIt++;
        }
    }

    //ADD REMAININGS
    while (sourceIt != source_data.end()) {
        cout << "\tAdded id " << sourceIt->first << std::endl;
        ret_data.insert(std::pair<string, XmlData>(sourceIt->first, sourceIt->second));
        sourceIt++;
    }

    while (targetIt != target_data.end()) {
        cout << "\tId " << targetIt->first << " not found in source" << std::endl;
        ret_data.insert(std::pair<string, XmlData>(targetIt->first, targetIt->second));
        targetIt++;
    }


    return ret_data;
}

int premerge_test(MergeData mergeData, std::map<string, XmlData>& output_data) {
    FILE* file_ptr_holder;
    int error_f{};
    std::map<string, XmlData> source_data, target_data;
    if ((error_f = fopen_s(&file_ptr_holder, mergeData.sourceFile.c_str(), "rb")) != 0) {
        show_error(std::to_string(error_f).append(" - open source file"), ERROR_R_XML);
        return ERROR_R_XML;
    }
    else if ((error_f = loadXmlToMap(&file_ptr_holder, source_data)) != 0) {
        show_error(std::to_string(error_f).append(" - loading xml from source "), ERROR_R_XML);
        if (file_ptr_holder)
            fclose(file_ptr_holder);
        return ERROR_R_XML;
    }
    if (file_ptr_holder)
        fclose(file_ptr_holder);

    if ((error_f = fopen_s(&file_ptr_holder, mergeData.targetFile.c_str(), "rb")) != 0) {
        show_error(std::to_string(error_f).append(" - open target file"), ERROR_R_XML);
        return ERROR_R_XML;
    }
    else if ((error_f = loadXmlToMap(&file_ptr_holder, target_data)) != 0) {
        show_error(std::to_string(error_f).append(" - loading xml from target "), ERROR_R_XML);
        if (file_ptr_holder)
            fclose(file_ptr_holder);
        return ERROR_R_XML;
    }
    if (file_ptr_holder)
        fclose(file_ptr_holder);
    
    if (error_f == 0)
        output_data = mergeXmlData(source_data, target_data, mergeData);
    return error_f;
}

int create_xml_file(std::map<string, XmlData> data, FILE* output_data) {
    int ret_value{};
    //CREATE XML FILE
    XMLDocument doc;
    doc.Clear();
    doc.InsertFirstChild(doc.NewDeclaration());
    doc.InsertEndChild(doc.NewElement(strings_literal));

    for (auto finalIt = data.begin(); finalIt != data.end(); finalIt++) {
        if (doc.RootElement() == nullptr) {
            cout << "Root Element has not been initialized";
        }
        else {
            XMLElement* xmlElement = doc.RootElement()->InsertNewChildElement("string");
            xmlElement->SetAttribute("id", finalIt->first.c_str());
            xmlElement->SetText(finalIt->second.idValue.c_str());
            for (auto attr{ finalIt->second.attributes.begin() }; attr != finalIt->second.attributes.end(); attr++) {
                xmlElement->SetAttribute(std::get<0>(*attr).c_str(), std::get<1>(*attr).c_str());
            }
        }
    }
    XMLError xmlError = doc.SaveFile(output_data);
    if (xmlError != 0) {
        string err_text{ "Saving file /" };
        err_text.append(XMLDocument::ErrorIDToName(xmlError));
        show_error(err_text, ERROR_R_XML);
        ret_value = 1;
    }

    return ret_value;
}

void create_config(vector<string> filepath_array, string directory, string filename) {
    const char kPathSeparator{ '/' };
    const string default_filename{ "file_conf.conf" };
    const string extension_name{ ".conf" };

    std::size_t found{ filename.find('.') };
    if (found != string::npos){
        if (!filename.substr(found, string::npos)._Equal(extension_name)) {
            cout << "-> filename doesn't have " << extension_name << " extension" << std::endl;
            cout << "-> The default filename will be used instead: " << default_filename << std::endl;
            filename = default_filename;
        }
    }
    else {
        filename.append(extension_name);
    }

    if (directory.back() != kPathSeparator) {
        directory.push_back(kPathSeparator);
    }

    if (fs::is_directory(directory)) {
        directory.append(filename);

        std::ofstream confFile(directory);
        for (unsigned int i = 0; i < filepath_array.size(); i++) {
            confFile << filepath_array.at(i);
            if (i < filepath_array.size() - 1) {
                confFile << std::endl;
            }
        }
        confFile.close();
        if (!confFile.good()) show_error("create config", ERROR_W_FILE);
        else cout << "File " << filename << " created in " << directory << std::endl;
    }
    else show_error("create config", DIR_ISNOT_FOLDER);
}

int read_config(vector<string>& target_filepatch, string config_dir) {
    const string default_ext{ ".conf" };
    int ret_value{};
    std::size_t ext_pos{ config_dir.find_last_of('.') };
    if (ext_pos == string::npos) ret_value = 1;//show_error("read config", INVALID_FILE_EXT);
    else if (config_dir.substr(ext_pos, config_dir.size()) != default_ext) ret_value = 2;//show_error("read config", INVALID_FILE_EXT);
    else if (!fs::exists(config_dir)) ret_value = 3; // show_error(config_dir, FILE_DOESNT_EXIST);
    else if (!fs::is_regular_file(config_dir)) ret_value = 4; //show_error(config_dir, INVALID_FILE);
    else {
        std::ifstream in_stream(config_dir);
        while (!in_stream.eof()) {
            string temp{};
            getline(in_stream, temp);
            target_filepatch.push_back(temp);
        }
    }

    return ret_value;
}

void show_help(vector<string> args) {
    if (args.size() > 2) return show_error(args.at(0), TOO_MANY_PARAMS);

    if (args.size() == 2) {
        string extra_param{args.at(1)};
        if (strcmp("-help", extra_param.c_str()) == 0) {
            cout << "-help <command (optional)>" << std::endl;
            cout << "\tIf <command> is not informed it will show a list of the available commands." << std::endl;
            cout << "\tIf <command> is informed it will what <command> does." << std::endl;
        }
        else if (strcmp("-mkconf", extra_param.c_str()) == 0) {
            cout << "-mkconf <save directory> <config filename> <target 1> <target 2>..." << std::endl;
            cout << "\tThis command creates a config file with the targets directories for reuse." << std::endl;
            cout << "\tThe config file allows for multiple merges in one go." << std::endl;
            cout << "\t<save directory> must be a directory, not a file." << std::endl;
            cout << "\t<config filename> must'nt have an extension or have the '.conf' extension." << std::endl;
            cout << "\t<target x> directory of the targets (filename & ext included), the numbers of targets you can add is not limited." << std::endl;
        }
        else if (strcmp("-merge", extra_param.c_str()) == 0) {
            cout << "-merge <-kta (optional)> <target> <source> <output(optional)>" << std::endl;
            cout << "\t<-kta> if not used the attributes of the target will not appear on the result file." << std::endl;
            cout << "\tmerges <target> and <source>, the result will be saved in <output> file." << std::endl;
            cout << "\tIf <output> is not informed the result of the merge will be saved in <target>" << std::endl;
        }
        else if (strcmp("-mergeconf", extra_param.c_str()) == 0) {
            cout << "-mergeconf <conf file directory> <-kta (optional)>" << std::endl;
            cout << "\tStarts the process of batch merge." << std::endl;
            cout << "\t<conf file directory> will be readed and for each directory in the conf file will ask for a source." << std::endl;
            cout << "\t<-kta> if used it will merge the target's attrs with the source's attrs, if not used it will ignore the target's attrs and use only the source's attrs" << std::endl;
            cout << "\tEach batch has a capacity of " << TMP_MAX_S << ", when it's full capacity is reached the changes will be commited." << std::endl;
        }
        else if (strcmp("-cls", extra_param.c_str()) == 0) {
            cout << "-cls" << std::endl;
            cout << "\tclears the scree.n" << std::endl;
        }
        else if (strcmp("-loop", extra_param.c_str()) == 0) {
            cout << "-loop" << std::endl;
            cout << "\tEnables/disables loop." << std::endl;
        }
        else if (strcmp("-v", extra_param.c_str()) == 0) {
            cout << "-v" << std::endl;
            cout << "\tshows info about this program." << std::endl;
        }
        else {
            show_error(args.at(0).append(" ").append(args.at(1)), UNKNOWN_ARGUMENT);
        }
    }
    else {
        //show_header();
        cout << std::endl;
        cout << "List of commands available:" << std::endl;
        cout << "\t" << "-mkconf <save directory> <config filename> <directory target 1> <dir target 2>..." << std::endl;
        cout << "\t" << "-merge <-kta (optional)> <target file dir> <source file dir> <output file dir (optional)>" << std::endl;
        cout << "\t" << "-mergeconf <conf file directory> <-kta (optional)>" << std::endl;
        cout << "\t" << "-cls" << std::endl;
        cout << "\t" << "-cls" << std::endl;
        cout << "\t" << "-v" << std::endl;
        cout << "\t" << "-help <command (optional)>" << std::endl;
    }
}

void mkconfig_command(vector<string> args) {
    int ret_value{};
    if (args.size() < 4) {
        show_error(args.at(0), TOO_FEW_PARAMS);
    }
    else {
        string save_directory{args.at(1)};
        string config_filename{args.at(2)};

        vector<string> targets{};
        for (unsigned int i = 3; i < args.size(); i++) {
            targets.push_back(args.at(i));
        }

        return create_config(targets, save_directory, config_filename);
    }
}

void merge_command(vector<string> args) {
    if (args.size() < 3) show_error(args.at(0), TOO_FEW_PARAMS);
    else if ((!args.at(1)._Equal("-kta") && args.size() > 4) || (args.at(1)._Equal("-kta") && args.size() > 5)) show_error(args.at(0), TOO_MANY_PARAMS);
    else {
        MergeData data;
        int index{ 1 };
        if (args.at(1)._Equal("-kta")) {
            data.keep_target_attrs = true;
            index++;
        }
        data.targetFile = args.at(index++);
        data.sourceFile = args.at(index++);
        data.outputFile = (args.size() == index) ? data.targetFile : args.at(index);
        std::map<string, XmlData> output_data;
        int error{};
        if ((error = premerge_test(data, output_data)) == 0) {
            FILE* output_file;
            if ((error = fopen_s(&output_file, data.outputFile.c_str(), "w+")) == 0) {
                error = create_xml_file(output_data, output_file);
            }
            else {
                show_error(args.at(0).append(" - ").append(data.outputFile), INVALID_FILE);
            }
            if (output_file)
                fclose(output_file);
        }

        if (error != 0) {
            show_error(args.at(0), UNKNOWN_ERROR);
        }
        else
        {
            cout << "Merge Successful!" << std::endl;
        }
    }
}

void merge_conf_command(vector<string> args) {
    if (args.size() < 2) show_error("-mergeconf", TOO_FEW_PARAMS);
    else if (args.size() > 3) show_error("-mergeconf", TOO_MANY_PARAMS);
    else {
        vector<string> target_files;
        int read_f_error;
        if ((read_f_error = read_config(target_files, args.at(1))) == 0 && target_files.size() > 0) {
            cout << "Conf file loaded." << std::endl;


            MergeData mergeData;
            //READ EXTRA PARAM
            bool kta{ false };
            if (args.size() == 3) {
                if (!args.at(2)._Equal("-kta")) {
                    std::stringstream ss;
                    ss << args.at(0) << " " << args.at(2);
                    show_error(ss.str(), UNKNOWN_ARGUMENT);
                    return;
                }
                else {
                    kta = true;
                    mergeData.keep_target_attrs = true;
                }
            }

            //GET SOURCE FILES FROM USER INPUT
            vector<string> source_files;
            for (unsigned int i{}; i < target_files.size(); i++) {
                cout << "Source file for " << target_files.at(i) << std::endl << "Source file " << (i + 1) << ": ";
                if (get_argument_from_cin(source_files) != 0) {
                    show_error("-mergeconf source file", INPUT_ERROR);
                    return;
                }
                else if (source_files.size() > (i + 1)) {
                    show_error("-mergeconf source file", TOO_MANY_PARAMS);
                    return;
                }
                else if (source_files.size() < (i + 1)) {
                    show_error("-mergeconf source file", TOO_FEW_PARAMS);
                    return;
                }
            }

            vector<FILE*> tmpfile_batch;
            unsigned int index{};
            int error_merging{};
            std::map<string, XmlData> dataholder;
            while (index < target_files.size() && error_merging == 0) {
                for (unsigned int i{ index }; i < index + TMP_MAX_S && i < target_files.size(); i++) {
                    mergeData.sourceFile = source_files.at(i);
                    mergeData.targetFile = target_files.at(i);

                    FILE* tmpFile{};
                    if ((error_merging = premerge_test(mergeData, dataholder)) == 0) {//merge data
                        if ((error_merging = tmpfile_s(&tmpFile)) == 0) { //create tmp file
                            if ((error_merging = create_xml_file(dataholder, tmpFile)) == 0) { //save data in tmp
                                fflush(tmpFile);
                                rewind(tmpFile);
                                tmpfile_batch.push_back(tmpFile);
                            }
                        }
                    }

                    if (error_merging != 0) {
                        if (tmpFile)
                            fclose(tmpFile);
                        break;
                    }
                }
                
                if(error_merging == 0) { //start merging batch if everything went ok
                    char char_list[BUFFER_SIZE]{};
                    std::streamsize num_read{};
                    for (unsigned int i = 0; i < tmpfile_batch.size(); i++) {
                        std::ofstream dest(target_files.at(index + i), std::ios::binary);
                        FILE* tmpFile{ tmpfile_batch.at(i) };
                        do {
                            num_read = fread_s(char_list, BUFFER_SIZE, ELEMENT_SIZE, BUFFER_SIZE, tmpFile);
                            dest.write(char_list, num_read);
                        } while (!(feof(tmpFile) || ferror(tmpFile) || dest.fail() || dest.bad()));
                        if ( ferror(tmpFile) || dest.fail() || dest.bad()) {
                            cout << "An error has ocurred writing in " << target_files.at(index + i) << std::endl;
                            cout << "ferror: " << ferror(tmpFile) << std::endl;
                            cout << "- dest.fail: " << dest.fail();
                            cout << "- dest.bad: " << dest.bad() << std::endl;
                            error_merging = 1;
                        }

                        dest.close();
                        if (tmpfile_batch.at(i))
                            fclose(tmpfile_batch.at(i));
                    }
                }

                //in case of error close all tmp files
                if (error_merging != 0) {
                    int batchs_merged = index / TMP_MAX_S;

                    cout << "**ERROR**" << std::endl;
                    cout << "\tError while merging files";
                    if (batchs_merged > 0) {
                        cout << ", couldnt rollback " << index / TMP_MAX_S << " batchs of " << TMP_MAX_S << " files." << std::endl;
                    }
                    else {
                        cout << ", no files were merged" << std::endl;
                    }
                    for (unsigned int i = 0; i < tmpfile_batch.size(); i++) {
                        if (tmpfile_batch.at(i)) //close if not null
                            fclose(tmpfile_batch.at(i));
                    }
                }


                index = (index + TMP_MAX_S < target_files.size()) ? index + TMP_MAX_S : target_files.size();
                tmpfile_batch.clear();
            }
            
            if (error_merging == 0) {
                cout << "All files were merged!" << std::endl;
            }
        }
        else if (read_f_error == 1) show_error("read config", INVALID_FILE_EXT);
        else if (read_f_error == 2) show_error("read config", INVALID_FILE_EXT);
        else if (read_f_error == 3) show_error("read config", FILE_DOESNT_EXIST);
        else if (read_f_error == 4) show_error("read config", INVALID_FILE);
        else if (read_f_error == 0 && target_files.size() == 0) show_error("read config", EMPTY_CONF_FILE);
        else show_error("read config", UNKNOWN_ERROR);

    }
}

void argument_function(vector<string> args) {

    int ret_value{};
    string arg_readed{};
    string extra_param{};
    bool loop{ false };
    do {
        arg_readed = args.at(0);
        if (strcmp("-help", arg_readed.c_str()) == 0) show_help(args);
        else if (strcmp("-mkconf", arg_readed.c_str()) == 0) mkconfig_command(args);
        else if (strcmp("-merge", arg_readed.c_str()) == 0) merge_command(args);
        else if (strcmp("-mergeconf", arg_readed.c_str()) == 0) merge_conf_command(args);
        else if (strcmp("-v", arg_readed.c_str()) == 0) show_header();
        else if (strcmp("-cls", arg_readed.c_str()) == 0) system("CLS");
        else if (strcmp("-loop", arg_readed.c_str()) == 0) { 
            loop = !loop;
            if (loop) cout << "Loop started!" << std::endl;
            else  cout << "Loop ended!" << std::endl;
        }
        else show_error("argument function", UNKNOWN_ARGUMENT);

        if (loop) {
            args.clear();
            int error{};
            do {
                error = loop_get_argument_from_cin(args);
                if (error != 0 && error != 3) {
                    show_error(std::to_string(error), INPUT_ERROR);
                    loop = false;
                }
            } while (error == 3);
        }
    } while (loop);
}

void test_error() {
    int num{};
    cin >> num;
    if (num >= 1 && num <= 11) {
        show_error("test error " + std::to_string(num), static_cast<AssmtErrorEnum>(num));
    }
}

int main(int argc, char* argv[]) {
    show_header();

    int ret_value{};
    
    vector<string> arguments;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            arguments.push_back(argv[i]);
        }
    }
    else {
        if (loop_get_argument_from_cin(arguments) != 0) show_error("main", INPUT_ERROR);
    }
    argument_function(arguments);
    
    return ret_value;
}