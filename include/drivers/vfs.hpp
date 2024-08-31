#pragma once

namespace vfs {
    struct dentry {
    };

    struct fs {
        fs(const char*);

        virtual void mount();
    };
}