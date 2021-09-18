import os


def get_share():
    zephyr_cosim_dir = os.path.dirname(os.path.abspath(__file__))
    
    if os.path.isdir(os.path.join(zephyr_cosim_dir, "share")):
        # This is an installation
        return os.path.join(zephyr_cosim_dir, "share")
    else:
        # Go up two more dirs to the project root
        proj_dir = os.path.dirname(os.path.dirname(zephyr_cosim_dir))
        return os.path.join(proj_dir, "share")