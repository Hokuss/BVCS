# BVCS - Basic Version Control System

## An audacious attempt to dethrone Git. (On a serious note: a very simplified and readable basic version control system.)

![Gemini_Generated_Image_9l18fz9l18fz9l18](https://github.com/user-attachments/assets/23bce804-34c7-4ee2-896a-df0b3c816db7)

## 🚀 Welcome to BVCS!

Tired of the complex incantations, the arcane ceremonies, and the bewildering array of options that modern version control systems present? Do you yearn for a simpler time, when tracking changes felt... intuitive?

BVCS is born from that yearning. It's not designed to manage the Linux kernel, nor to handle thousands of concurrent developers. Instead, it's an educational project, a thought experiment, and a genuinely humble attempt to demonstrate the core principles of version control in the most readable, approachable, and *basic* way possible.

I believe that understanding the fundamentals doesn't require a deep dive into cryptographic hashes, DAGs, or rebase vs. merge philosophical debates. BVCS aims to be a stepping stone, a "hello world" for version control, proving that even a basic system can provide immense value.

**Can it dethrone Git?** Probably not today. But it might just dethrone your fear of version control concepts!

## ✨ Working Commands/Features

* **`bvcs start`**: Initialize a new BVCS repository in your project directory. Creates a `.bvcs` folder. Also stages the initial state of the project.
* **`bvcs change`**: Stage changes of all the changes in project (if any).
* **`bvcs next`**: Record staged changes as a new "version". Each version only stores the changes made to the project except for the new one.
* **`bvcs fetch`**: Replaces the current version of the project with current commit fetched by version.
* **`bvcs version <version_id>`**: Revert your working directory to a previous version.
* **`bvcs dismantle`**: Deletes the BVCS parts from the project.
* LZ4 Compression: All stored version data (the differences) are automatically compressed using the fast LZ4 algorithm to ensure efficient storage and faster operations.

## ✨ Future Features

* Multiple Branch Support: Enable multiple contributors to work concurrently on the project and then merge their changes, especially when different files are modified across branches.
* Optimizations: Enhance performance and storage efficiency as the project evolves.
* Community Input: Your comments and suggestions are welcome and will be considered for future development!

## 🛠️ How it (Conceptually) Works

BVCS stores versions by creating simple, version number directories within the `.bvcs/version_history` folder. Each commit just stores the diffs (entire file/folder structure  that has changed) between the last version and current state of the project at the time of snapshot.

* `.bvcs/`: The root BVCS directory.
    * `config`: Stores basic repository configuration (e.g., just the version number for the time being).
    * `staging_area/`: Stores copies of files that have been "added/modified" (staged).
    * `version_history/`: Stores the differences.
        * `<timestamp_or_hash_0>/`: A full copy of the project at the initial start commit.
        * `<timestamp_or_hash_n>/`: Contains the differences (changed files/folders) relative to the previous version, compressed with LZ4.
    * `current_commit`: A current version of the project as per last snapshot.

## 🚀 Getting Started (Conceptual Usage)

This section will be added soon, providing clear examples and step-by-step instructions on how to use BVCS commands.
