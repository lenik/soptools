test_exe = executable(
    'stream_copy_test',
    [
        'stream_copy_test.cpp',
        '../src/util/stream_copy.cpp',
    ],
    include_directories: include_directories('../src'),
    install: false,
)

model_test = executable(
    'model_test',
    [
        'sop_model_test.cpp',
        '../src/model/model.cpp',
        '../src/model/model_parse.cpp',
    ],
    include_directories: include_directories('../src'),
    cpp_args: ['-DTEST_SOP_DIR="@0@"'.format(meson.project_source_root() / 'worldman.sop')],
    install: false,
)

test('stream_copy', test_exe)
test('model', model_test, workdir: meson.current_build_dir())

project_test = executable(
    'project_test',
    [
        'sop_project_test.cpp',
        '../src/engine/project.cpp',
        '../src/engine/engine.cpp',
        '../src/engine/shell.cpp',
        '../src/engine/apply.cpp',
        '../src/model/model.cpp',
        '../src/model/model_parse.cpp',
        '../src/util/paths.cpp',
        '../src/engine/ai_output.cpp',
        '../src/engine/gpt_response.cpp',
    ],
    include_directories: include_directories('../src', '..'),
    cpp_args: ['-DTEST_SOP_DIR="@0@"'.format(meson.project_source_root() / 'worldman.sop')],
    install: false,
)

test('project', project_test, workdir: meson.current_build_dir())
