{
    "targets": [{
        "target_name": "vcam",
        "sources": [
            "src/vcam.cpp",
        ],
        "include_dirs" : [
            "<!(node -e \"require('nan')\")"
        ]
    }]
}