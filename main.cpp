#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}


bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {
    path parent_dir = in_file.parent_path();
    static regex num_reg1(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    static regex num_reg2(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");   
    smatch m1;
    smatch m2;
    ifstream main_in(in_file.string(), ios::in);
    if (!main_in) {
        return false;
    }
    ofstream main_out(out_file.string(), ios::out | ios::app);
    if (!main_out) {
        return false;
    }
    string s;
    int iterator = 0;
    bool is_find_dir = true;
    while (getline(main_in, s)) {
        iterator++;
        if (regex_match(s, m1, num_reg1)) {
            path p = parent_dir / string(m1[1]);
            ifstream in_include(p);
            if (in_include.is_open()) {
                Preprocess(p, out_file, include_directories);
                in_include.close();
            } else {
                is_find_dir = false;
                for (const auto& dir : include_directories) {
                    p = dir / string(m1[1]);
                    in_include.open(p);
                    if (in_include.is_open()) {
                        Preprocess(p, out_file, include_directories);
                        in_include.close();
                        is_find_dir = true;
                        break;
                    }
                }
                if (!is_find_dir) {
                    cout << "unknown include file "s << string(m1[1]) << " at file "s << in_file.string() << " at line "s << iterator<< endl;
                    return is_find_dir;
                }    
            }
        } else if (regex_match(s, m2, num_reg2)) {
            is_find_dir = false;
            for (const auto& dir : include_directories) {
                path p = dir / string(m2[1]);
                ifstream in_include(p);
                if (in_include.is_open()) {
                    Preprocess(p, out_file, include_directories);
                    in_include.close();
                    is_find_dir = true;
                    break;
                    }
                }
            if (!is_find_dir) {
                cout << "unknown include file "s << string(m2[1]) << " at file "s << in_file.string() << " at line "s << iterator<< endl;
                return is_find_dir;
            }
        } else {
            main_out << s << endl;
        }
    }
    return is_find_dir;
}

string GetFileContents(string file) {
    ifstream stream(file);

    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}