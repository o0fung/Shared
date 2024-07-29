# Working Instruction:

## Table of Contents
1. [Introduction](#introduction)
2. [Prerequisites](#prerequisites)
3. [Setup](#setup)
4. [Workflow](#workflow)
   - [Cloning the Repository](#cloning-the-repository)
   - [Creating a Branch](#creating-a-branch)
   - [Making Changes](#making-changes)
   - [Staging Changes](#staging-changes)
   - [Committing Changes](#committing-changes)
   - [Pushing Changes](#pushing-changes)
   - [Pulling Changes](#pulling-changes)
   - [Creating a Pull Request](#creating-a-pull-request)
5. [Best Practices](#best-practices)
6. [Troubleshooting](#troubleshooting)
7. [Resources](#resources)

## Introduction
This repository is for collaborating on Shared git folder. Here you'll find the instructions for setting up your environment and contributing to the project.

## Prerequisites
- Git installed on your Windows machine. You can download it from [git-scm.com](https://git-scm.com/).
- A GitHub account.
- Access to the repository.

## Setup
1. **Install Git**: Follow the installation instructions on the [Git website](https://git-scm.com/downloads).
2. **Configure Git**: Open Git Bash (or your preferred terminal) and set your username and email.
   ```bash
   git config --global user.name "your_name"
   git config --global user.email "your_email_address@your_email_server.com"
   ```

## Workflow

### Cloning the Repository
1. Open Git Bash.
2. Navigate to the directory where you want to clone the repository.
   ```bash
   cd target/path/on/your/computer
   ```
3. Clone the repository.
   ```bash
   git clone https://github.com/o0fung/shared.git
   ```
4. Navigate into the repository.
   ```bash
   cd repository
   ```

### Creating a Branch
1. Pull the latest changes from the main branch.
   ```bash
   git pull origin main
   ```
2. Create a new branch for your work.
   ```bash
   git checkout -b your-branch-name
   ```

### Making Changes
1. Make your changes to the codebase using your preferred text editor or IDE.

### Staging Changes
1. Check the status of your changes.
   ```bash
   git status
   ```
2. Stage the changes you want to commit.
   ```bash
   git add .
   ```

### Committing Changes
1. Commit your changes with a descriptive message.
   ```bash
   git commit -m "Brief description of your changes"
   ```

### Pushing Changes
1. Push your changes to your branch on GitHub.
   ```bash
   git push origin your-branch-name
   ```

### Pulling Changes
1. Before starting new work, always pull the latest changes from the main branch.
   ```bash
   git checkout main
   git pull origin main
   ```
2. Merge the latest changes into your branch.
   ```bash
   git checkout your-branch-name
   git merge main
   ```

### Creating a Pull Request
1. Go to the GitHub repository in your web browser.
2. Click the "Pull requests" tab.
3. Click the "New pull request" button.
4. Select your branch and compare it with the main branch.
5. Add a title and description for your pull request.
6. Click "Create pull request".

## Best Practices
- Commit frequently with clear messages.
- Pull the latest changes before starting new work.
- Test your changes locally before pushing.
- Review pull requests thoroughly.

## Troubleshooting
- **Merge Conflicts**: If you have conflicts, Git will notify you. Open the conflicting files, resolve the conflicts, then add and commit the resolved files.
  ```bash
  git add .
  git commit -m "Resolved merge conflicts"
  ```

## Resources
- [Git Documentation](https://git-scm.com/doc)
- [GitHub Help](https://docs.github.com/en)
