import os
import git
from argparse import Action, ArgumentTypeError
from git import Repo
from gitdb.exc import BadName
import tempfile

class ReleaseRepoAction(Action):
        def __call__(self, parser, namespace, repo_at_commit_id, option_string=None):
                repo, commit = self.validate(repo_at_commit_id[0])
                release = {"commit": commit, "name":repo}
                setattr(namespace, self.dest, release)
        
        def validate(self, repo_at_commit_id):
                [name, commit] = repo_at_commit_id.split('@')
                url = "git@github.com:bespoke-silicon-group/{}.git".format(name)
                with tempfile.TemporaryDirectory(dir="/tmp/") as d:
                        r = git.Repo.clone_from(url, d)
                        try:
                                r.tree(commit)
                        except BadName:
                                raise ValueError("Commit ID argument {} is not in tree of {}".format(commit, name))
                return (name, commit)
