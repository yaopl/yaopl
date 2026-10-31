# SMB share

YAOPL can load games from a shared folder on a PC or NAS over the LAN (the *ETH Games* page). It uses the old
**SMBv1** protocol, which current systems turn off by default, so the share needs some setup.

## On the PC

Create a folder for YAOPL (for example `PS2SMB`) and share it with read and write permissions. YAOPL creates its
folders in it (`CD`, `DVD`, `CFG`, `VMC`, ...) the first time it connects; copy your games into `DVD` and `CD`
as described in [Adding games](../games.md).

=== "Windows"

    1. Turn on SMBv1: *Turn Windows features on or off* → *SMB 1.0/CIFS File Sharing Support* →
       *SMB 1.0/CIFS Server*. Restart the PC.
    2. Right-click the folder → *Properties* → *Sharing* → *Advanced Sharing*: share it, and under
       *Permissions* give the account you will use *Full Control*.
    3. Use a Windows account with a password for YAOPL (*User* and *Password* below). Guest access without a
       password is often blocked by Windows.
    4. Give the PC a fixed address (or a DHCP reservation in the router), so the PS2 always finds it.

=== "Linux (Samba)"

    Allow SMB1 and the old NTLM logon in `/etc/samba/smb.conf`, and add a share:

    ```ini
    [global]
       server min protocol = NT1
       ntlm auth = ntlmv1-permitted

    [PS2SMB]
       path = /srv/ps2smb
       read only = no
       guest ok = yes
       force user = youruser
    ```

    Restart Samba (`sudo systemctl restart smbd`). With `guest ok = yes`, leave *User* and *Password* empty in
    YAOPL; otherwise add a Samba user with `sudo smbpasswd -a youruser`.

=== "NAS"

    Turn on SMB1 (sometimes called "SMB 1", "CIFS" or "minimum SMB protocol: SMB1") and NTLMv1 in the NAS's
    file-sharing settings, and create a share with read and write access.

!!! warning "SMBv1 on your network"
    SMBv1 is old and less secure. Allow it only on a trusted home network, and only for this share if your
    system lets you.

## On the PS2

1. Main menu → **Network Settings**. Set up the PS2's address as in [Network setup](network.md).
2. In the *SMB Server* part:

    | Field | Default | What to enter |
    | --- | --- | --- |
    | Address Type | NetBIOS | **IP** (recommended) or NetBIOS |
    | Address | — | The PC's IP address, or its NetBIOS (computer) name, up to 16 characters |
    | Port | 445 | Change only with a server on another port (*Advanced Options*) |
    | Share | empty | The share name, for example `PS2SMB`. Leave empty to pick it from a list. |
    | User | empty | The account name; empty for guest access |
    | Password | empty | The account's password; empty for guest access |

3. Choose **OK**, then **Save Changes** in the main menu.
4. In **Settings**, set **ETH Start Mode** to Auto (or Manual) and save again.

The *ETH Games* page shows the games. With *Share* left empty, it lists the shares of the server instead:
pick one, and YAOPL connects to it. Save Changes to remember it.

After changing the SMB settings while connected, choose **Reconnect** in Network Settings. When the
connection failed, the OK button itself says Reconnect.

*ETH Prefix Path* in Settings makes YAOPL use a folder inside the share instead of its root.

## Errors

| Message | What to check |
| --- | --- |
| `300: Cannot connect to SMB server.` | The address; that the PC is on and SMBv1 is enabled; the firewall (port 445). With NetBIOS, the name could not be found: try the IP address. |
| `301: Cannot log into SMB server.` | User and password; on Linux, `ntlm auth`. |
| `303: Cannot open SMB share.` | The share name and its permissions. |
| `304: Cannot list SMB shares.` | Listing is not allowed by the server: enter the share name. |

Network errors (200, 310, 311) are on the [Network setup](network.md#network-errors) page.

## A game freezes after an idle time

A game is paused, or waits on a menu, and freezes at its next disc read. With a Samba server, its NetBIOS
keepalive messages pile up in the small network buffer pool of the PS2 while the game reads nothing; after
about eight of them the PS2 stops answering and Samba drops the connection (upstream issue #79). Turn these
keepalives off and let TCP keep the connection alive instead: in the `[global]` section of `smb.conf` set

```ini
keepalive = 0
socket options = SO_KEEPALIVE=1 TCP_NODELAY=1
```

and restart Samba.

## Notes

- SMB is slower than an internal HDD; in some games long videos can stutter. [UDPBD](udpbd.md) is a faster
  network alternative.
- SMB and UDPBD cannot run at the same time.
- The SMB password is stored in plain text in `conf_network.cfg`.
