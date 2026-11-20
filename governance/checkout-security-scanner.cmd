@echo off
rem Enable the checkout of the governance/security-scanner submodule (disabled by default) and fetch it
setlocal
set "ROOT=%~dp0.."
git -C "%ROOT%" config submodule.governance/security-scanner.update checkout || exit /b 1
git -C "%ROOT%" submodule update --init governance/security-scanner || exit /b 1
