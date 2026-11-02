# The CI workflow calls this script before the build. This branch takes the NGX DLLs from the packman package, as meson.build
# reads them, so there is no pinned bundle to fetch.
Write-Host "No pinned DLSS / NGX bundle on this branch."
