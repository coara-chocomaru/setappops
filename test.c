#include <stdlib.h>

int main() {
    system("settings put secure install_non_market_apps 1 > /dev/null 2>&1");
    system("settings put secure unknown_sources 1 > /dev/null 2>&1");
    system("settings put global install_non_market_apps 1 > /dev/null 2>&1");
    system("settings put global verifier_verify_adb_installs 0 > /dev/null 2>&1");
    system("settings put global package_verifier_enable 0 > /dev/null 2>&1");
    system("settings put secure installer_allow_from_unknown_sources 1 > /dev/null 2>&1");
    system("settings put global hidden_api_policy_pre_p_apps 1 > /dev/null 2>&1"); 
    system("settings put global hidden_api_policy_p_apps 1 > /dev/null 2>&1");
    system("pm set-install-location 0 > /dev/null 2>&1");
    system("pm disable-verification > /dev/null 2>&1");

    return 0;
}
