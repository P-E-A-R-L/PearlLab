#ifndef INSPECTOR_HPP
#define INSPECTOR_HPP

namespace Pipeline {
    struct VisualizedAgent;
}

namespace Inspector {
    void init();
    void update();
    void onStart();
    void render();
    void onStop();
    void destroy();

    void open(Pipeline::VisualizedAgent*);
}

#endif // INSPECTOR_HPP
