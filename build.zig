const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    _ = b.addExecutable(.{
        .name = "ferment",
        .target = target,
        .root_source_file = b.path("main.c"),
        .optimize = optimize,
    });

    // Linux AMD64 Build
    const linux_amd64_build = b.addSystemCommand(&.{
        "zig",                     "cc",
        "-target",                 "x86_64-linux-gnu",
        "-O3",                     "-o",
        "bin/ferment-linux-amd64", "main.c",
    });

    // Linux AArch64 Build
    const linux_aarch64_build = b.addSystemCommand(&.{
        "zig",                       "cc",
        "-target",                   "aarch64-linux-gnu",
        "-O3",                       "-o",
        "bin/ferment-linux-aarch64", "main.c",
    });

    // Create a custom step to build all targets
    const build_all = b.step("build-all", "Build for all targets");
    build_all.dependOn(&linux_amd64_build.step);
    build_all.dependOn(&linux_aarch64_build.step);

    // Make the default build step build all targets
    b.getInstallStep().dependOn(build_all);
}
