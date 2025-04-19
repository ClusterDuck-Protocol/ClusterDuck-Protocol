Import("env")
import os

example_dir = os.getenv("EXAMPLE_DIR", "Basic-Ducks/MamaDuck")
env['PROJECT_SRC_DIR'] = env['PROJECT_DIR'] + "/examples/" + example_dir
env['PROJECTSRC_DIR'] = env['PROJECT_DIR'] + "/examples/" + example_dir

print("Setting the project directory to: {}".format(env['PROJECTSRC_DIR']))
