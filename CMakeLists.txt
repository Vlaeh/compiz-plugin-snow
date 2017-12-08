find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (snow PLUGINDEPS opengl composite)
