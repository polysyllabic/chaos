Chaosface
=========

Chaosface is the chatbot and user interface for Twitch Controls Chaos.

This repository includes:

* Twitch chatbot logic
* Streamer UI components
* Installation scripts for local and remote deployments

UI Access Protection
--------------------

Chaosface now supports optional password protection for the web UI.

First run behavior:

* On first startup, Chaosface opens a security setup page.
* Users can set a UI password, or choose to continue without one.
* If no password is entered, Chaosface shows a warning and requires explicit confirmation.
* The choice is persisted so users are not prompted again unless they change settings.

Login behavior:

* If password mode is enabled, each new browser session must log in.
* Passwords are not stored in plaintext; Chaosface stores an encrypted password-verifier payload.

HTTPS / TLS
-----------

Chaosface supports three UI TLS modes from Connection Setup:

* ``off``: HTTP only
* ``self-signed``: HTTPS using an auto-generated self-signed certificate
* ``custom``: HTTPS using a provided certificate and private key

Self-signed mode:

* Chaosface auto-generates files at ``configs/tls/selfsigned-cert.pem`` and
  ``configs/tls/selfsigned-key.pem`` if they do not already exist.
* You can configure the self-signed certificate hostname in Connection Setup.

Custom certificate mode:

* Provide paths to your PEM certificate and private key in Connection Setup.
* Both files must exist and be readable by the ``chaosface`` process.

Example file install (systemd deployment):

.. code-block:: bash

   sudo mkdir -p /usr/local/chaos/certs
   sudo cp fullchain.pem /usr/local/chaos/certs/chaosface-cert.pem
   sudo cp privkey.pem /usr/local/chaos/certs/chaosface-key.pem
   sudo chown root:root /usr/local/chaos/certs/chaosface-*.pem
   sudo chmod 600 /usr/local/chaos/certs/chaosface-key.pem
   sudo chmod 644 /usr/local/chaos/certs/chaosface-cert.pem

After changing password or TLS settings, restart the service:

.. code-block:: bash

   sudo systemctl restart chaosface
