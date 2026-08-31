test_exe = executable(
    'stream_copy_test',
    [
        'stream_copy_test.cpp',
        '../src/stream_copy.cpp',
    ],
    include_directories: include_directories('../src'),
    install: false,
)

sop_model_test = executable(
    'sop_model_test',
    [
        'sop_model_test.cpp',
        '../src/sop_model.cpp',
    ],
    include_directories: include_directories('../src'),
    cpp_args: ['-DTEST_SOP_DIR="@0@"'.format(meson.project_source_root() / 'figma.sop')],
    install: false,
)

test('stream_copy', test_exe)
test('sop_model', sop_model_test, workdir: meson.current_build_dir())

sop_project_test = executable(
    'sop_project_test',
    [
        'sop_project_test.cpp',
        '../src/sop_project.cpp',
        '../src/sop_runtime.cpp',
        '../src/sop_model.cpp',
        '../src/sop_paths.cpp',
        '../src/sop_ai_output.cpp',
    ],
    include_directories: include_directories('../src', '..'),
    cpp_args: ['-DTEST_SOP_DIR="@0@"'.format(meson.project_source_root() / 'figma.sop')],
    install: false,
)

test('sop_project', sop_project_test, workdir: meson.current_build_dir())
