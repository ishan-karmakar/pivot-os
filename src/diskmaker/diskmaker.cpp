#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <filesystem>
#include <unistd.h>
#include <cstring>
typedef std::map<std::string, std::string> config_t;

const std::string CONFIG_FILE("dmconfig");
config_t config;
const char *size = "";
char buf[100];

void parse_config();
size_t get_size(std::string);
void create_image();
std::string config_val(const char*);
void copy_file(std::ofstream&, std::ifstream, size_t, size_t, size_t, size_t);
inline void run_command(char *cmd);

int main(int argc, char *argv[]) {
    int c = 0;
    while ((c = getopt(argc, argv, "s:")) != -1)
        switch (c) {
        case 's':
            size = optarg;
            break;
        default:
            return 1;
        }
    parse_config();
    if (*size)
        std::cout << get_size(config_val(size)) << '\n';
    else
        create_image();

    return 0;
}

void parse_config() {
    std::ifstream file(CONFIG_FILE);
    std::string line;
    while (std::getline(file, line)) {
        if (line[0] == '#')
            continue;
        std::stringstream ss(line);
        std::string key;
        while (std::getline(ss, line, '=')) {
            if (key.empty())
                key = line;
            else {
                config[key] = line;
                break;
            }
        }
    }
}

void create_image() {
    size_t efi_sectors = get_size(config_val("EFI"));
    size_t elf_sectors = get_size(config_val("ELF"));
    size_t b2_sectors = get_size(config_val("BIOS2"));
    // Two additional sectors for padding
    size_t fat_sectors = 2 + 4 + b2_sectors + std::stoi(config_val("NUM_SUBDIRECTORIES")) + efi_sectors + elf_sectors;
    fat_sectors = std::max(69UL, fat_sectors); // 69 blocks seems to be the minimum number of sectors needed to satisfy MKFS
    size_t os_sectors = 34 + 33 + fat_sectors;
    std::string out_str = config_val("OUT");
    const char *out = out_str.c_str();

    std::remove("tmp.img");
    std::remove(out);

    std::sprintf(buf, "%s -f 1 -s 1 -a -r 16 -F12 -R %lu -C tmp.img %lu", config_val("FAT_MKFS").c_str(), b2_sectors + 1, (fat_sectors + 1) / 2);
    run_command(buf);

    auto create_subdirectories = [&](std::string dir_str, std::string file) {
        const char *fstr = "%s -i tmp.img ::%s";
        std::filesystem::path dir(dir_str);
        for (auto it = std::next(dir.begin()); it != dir.end(); it++) {
            std::filesystem::path ancestor;
            for (auto jt = dir.begin(); jt != std::next(it); jt++)
                ancestor /= *jt;

            std::sprintf(buf, "%s -i tmp.img ::%s", config_val("MMD").c_str(), ancestor.c_str());
            run_command(buf);
        }
        std::sprintf(buf, "%s -i tmp.img %s ::%s", config_val("MCOPY").c_str(), file.c_str(), dir_str.c_str());
        run_command(buf);
    };

    create_subdirectories(config_val("EFI_PATH"), config_val("EFI"));
    create_subdirectories(config_val("ELF_PATH"), config_val("ELF")); // PROBLEM

    std::ofstream ofs(out, std::ios::binary);
    std::filesystem::resize_file(out, os_sectors * 512);

    std::sprintf(buf, "%s -N 1 -t 1:ef00 -h 1 %s", config_val("SGDISK").c_str(), out);
    run_command(buf);

    copy_file(ofs, std::ifstream(config_val("BIOS1"), std::ios::binary), 0, 0, 446, 1);
    copy_file(ofs, std::ifstream(config_val("BIOS1"), std::ios::binary), 510, 510, 2, 1);
    copy_file(ofs, std::ifstream("tmp.img", std::ios::binary), 34, 0, fat_sectors, 512);
    copy_file(ofs, std::ifstream(config_val("BIOS2"), std::ios::binary), 35, 0, b2_sectors, 512);
}

void copy_file(std::ofstream& out, std::ifstream in, size_t seek, size_t skip, size_t size, size_t bs) {
    in.seekg(skip * bs);
    out.seekp(seek * bs);

    char *buf = new char[bs];
    for (size_t i = 0; i < size; i++) {
        in.read(buf, bs);
        out.write(buf, bs);
    }
}

// Returns file's size in sectors (512 bytes)
size_t get_size(std::string path) {
    std::ifstream in(path, std::ifstream::ate | std::ifstream::binary);
    return ((size_t) in.tellg() + 511) / 512;
}

std::string config_val(const char *key) {
    if (config.find(key) == config.end())
        throw std::invalid_argument("Could not find " + std::string(key) + " in config");
    return config[key];
}

inline void run_command(char *cmd) {
    std::strcat(cmd, " > /dev/null");
    std::system(cmd);
}
