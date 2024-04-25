TOP_LEVEL_DIR=$(git rev-parse --show-toplevel)

pushd .

cd $TOP_LEVEL_DIR/scripts

ruby sync_mac_project.rb

popd

